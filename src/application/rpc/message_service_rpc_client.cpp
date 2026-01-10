#include "application/rpc/message_service_rpc_client.hpp"

#include "core/system/application.hpp"

namespace IM::app::rpc {

namespace {
constexpr uint32_t kCmdLoadRecords = 301;
constexpr uint32_t kCmdLoadHistoryRecords = 302;
constexpr uint32_t kCmdLoadForwardRecords = 303;
constexpr uint32_t kCmdDeleteMessages = 304;
constexpr uint32_t kCmdDeleteAllMessagesInTalkForUser = 305;
constexpr uint32_t kCmdClearTalkRecords = 306;
constexpr uint32_t kCmdRevokeMessage = 307;
constexpr uint32_t kCmdSendMessage = 308;
constexpr uint32_t kCmdUpdateMessageStatus = 309;

constexpr uint32_t kTimeoutMs = 3000;

}  // namespace

MessageServiceRpcClient::MessageServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("message.rpc_addr", std::string(""), "svc-message rpc address ip:port")) {}

uint64_t MessageServiceRpcClient::resolveTalkId(const uint8_t /*talk_mode*/, const uint64_t /*to_from_id*/) {
    return 0;
}

bool MessageServiceRpcClient::buildRecord(const IM::model::Message & /*msg*/, IM::dto::MessageRecord & /*out*/,
                                          std::string *err) {
    if (err) {
        *err = "MessageServiceRpcClient::buildRecord not supported";
    }
    return false;
}

bool MessageServiceRpcClient::GetTalkId(const uint64_t /*current_user_id*/, const uint8_t /*talk_mode*/,
                                        const uint64_t /*to_from_id*/, uint64_t &talk_id, std::string &err) {
    talk_id = 0;
    err = "MessageServiceRpcClient::GetTalkId not supported";
    return false;
}

IM::RockResult::ptr MessageServiceRpcClient::rockJsonRequest(const std::string &ip_port, uint32_t cmd,
                                                             const Json::Value &body, uint32_t timeout_ms) {
    if (ip_port.empty()) {
        return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr, nullptr);
    }

    IM::RockConnection::ptr conn;
    {
        IM::RWMutex::ReadLock lock(m_mutex);
        auto it = m_conns.find(ip_port);
        if (it != m_conns.end() && it->second && it->second->isConnected()) {
            conn = it->second;
        }
    }

    if (!conn) {
        auto addr = IM::Address::LookupAny(ip_port);
        if (!addr) {
            return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr, nullptr);
        }
        IM::RockConnection::ptr new_conn(new IM::RockConnection);
        if (!new_conn->connect(addr)) {
            return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr, nullptr);
        }
        new_conn->start();
        {
            IM::RWMutex::WriteLock lock(m_mutex);
            m_conns[ip_port] = new_conn;
        }
        conn = new_conn;
    }

    IM::RockRequest::ptr req = std::make_shared<IM::RockRequest>();
    req->setSn(m_sn.fetch_add(1));
    req->setCmd(cmd);
    req->setBody(IM::JsonUtil::ToString(body));
    return conn->request(req, timeout_ms);
}

std::string MessageServiceRpcClient::resolveSvcMessageAddr() {
    const auto fixed = m_rpc_addr->getValue();
    if (!fixed.empty()) return fixed;

    if (auto sd = IM::Application::GetInstance()->getServiceDiscovery()) {
        std::unordered_map<std::string,
                           std::unordered_map<std::string, std::unordered_map<uint64_t, IM::ServiceItemInfo::ptr>>>
            infos;
        sd->listServer(infos);
        auto itD = infos.find("im");
        if (itD == infos.end()) {
            sd->queryServer("im", "svc-message");
            return "";
        }
        auto itS = itD->second.find("svc-message");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-message");
            return "";
        }
        // 简单挑一个
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }
    return "";
}

bool MessageServiceRpcClient::parseMessageRecord(const Json::Value &j, IM::dto::MessageRecord &out) {
    if (!j.isObject()) return false;
    out.msg_id = IM::JsonUtil::GetString(j, "msg_id");
    out.sequence = IM::JsonUtil::GetUint64(j, "sequence");
    out.msg_type = IM::JsonUtil::GetUint16(j, "msg_type");
    out.from_id = IM::JsonUtil::GetUint64(j, "from_id");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.is_revoked = IM::JsonUtil::GetUint8(j, "is_revoked");
    out.status = IM::JsonUtil::GetUint8(j, "status");
    out.send_time = IM::JsonUtil::GetString(j, "send_time");
    out.extra = IM::JsonUtil::GetString(j, "extra");
    out.quote = IM::JsonUtil::GetString(j, "quote");
    return !out.msg_id.empty();
}

bool MessageServiceRpcClient::parseMessagePage(const Json::Value &j, IM::dto::MessagePage &out) {
    if (!j.isObject()) return false;
    out.cursor = IM::JsonUtil::GetUint64(j, "cursor");
    out.items.clear();
    if (j.isMember("items") && j["items"].isArray()) {
        for (auto &it : j["items"]) {
            IM::dto::MessageRecord r;
            if (parseMessageRecord(it, r)) {
                out.items.push_back(std::move(r));
            }
        }
    }
    return true;
}

Result<void> MessageServiceRpcClient::callVoid(uint32_t cmd, const std::function<void(Json::Value &)> &fill) {
    Result<void> result;
    Json::Value req(Json::objectValue);
    fill(req);

    const auto addr = resolveSvcMessageAddr();
    auto rr = rockJsonRequest(addr, cmd, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-message unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }
    result.ok = true;
    return result;
}

Result<IM::dto::MessageRecord> MessageServiceRpcClient::SendMessage(
    const uint64_t current_user_id, const uint8_t talk_mode, const uint64_t to_from_id, const uint16_t msg_type,
    const std::string &content_text, const std::string &extra, const std::string &quote_msg_id,
    const std::string &msg_id, const std::vector<uint64_t> &mentioned_user_ids) {
    Result<IM::dto::MessageRecord> result;

    Json::Value req(Json::objectValue);
    req["current_user_id"] = (Json::UInt64)current_user_id;
    req["talk_mode"] = (Json::UInt)talk_mode;
    req["to_from_id"] = (Json::UInt64)to_from_id;
    req["msg_type"] = (Json::UInt)msg_type;
    req["content_text"] = content_text;
    req["extra"] = extra;
    req["quote_msg_id"] = quote_msg_id;
    req["msg_id"] = msg_id;
    if (!mentioned_user_ids.empty()) {
        Json::Value arr(Json::arrayValue);
        for (auto id : mentioned_user_ids) arr.append((Json::UInt64)id);
        req["mentioned_user_ids"] = arr;
    }

    const auto addr = resolveSvcMessageAddr();
    auto rr = rockJsonRequest(addr, kCmdSendMessage, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-message unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody())) {
        result.code = 500;
        result.err = "invalid svc-message response";
        return result;
    }
    IM::dto::MessageRecord rec;
    if (!out.isMember("data") || !parseMessageRecord(out["data"], rec)) {
        result.code = 500;
        result.err = "invalid message record";
        return result;
    }
    result.data = std::move(rec);
    result.ok = true;
    return result;
}

Result<IM::dto::MessagePage> MessageServiceRpcClient::LoadRecords(const uint64_t current_user_id,
                                                                  const uint8_t talk_mode, const uint64_t to_from_id,
                                                                  uint64_t cursor, uint32_t limit) {
    Result<IM::dto::MessagePage> result;

    Json::Value req(Json::objectValue);
    req["current_user_id"] = (Json::UInt64)current_user_id;
    req["talk_mode"] = (Json::UInt)talk_mode;
    req["to_from_id"] = (Json::UInt64)to_from_id;
    req["cursor"] = (Json::UInt64)cursor;
    req["limit"] = (Json::UInt)limit;

    const auto addr = resolveSvcMessageAddr();
    auto rr = rockJsonRequest(addr, kCmdLoadRecords, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-message unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody())) {
        result.code = 500;
        result.err = "invalid svc-message response";
        return result;
    }
    IM::dto::MessagePage page;
    if (!out.isMember("data") || !parseMessagePage(out["data"], page)) {
        result.code = 500;
        result.err = "invalid message page";
        return result;
    }
    result.data = std::move(page);
    result.ok = true;
    return result;
}

Result<IM::dto::MessagePage> MessageServiceRpcClient::LoadHistoryRecords(const uint64_t current_user_id,
                                                                         const uint8_t talk_mode,
                                                                         const uint64_t to_from_id,
                                                                         const uint16_t msg_type, uint64_t cursor,
                                                                         uint32_t limit) {
    Result<IM::dto::MessagePage> result;

    Json::Value req(Json::objectValue);
    req["current_user_id"] = (Json::UInt64)current_user_id;
    req["talk_mode"] = (Json::UInt)talk_mode;
    req["to_from_id"] = (Json::UInt64)to_from_id;
    req["msg_type"] = (Json::UInt)msg_type;
    req["cursor"] = (Json::UInt64)cursor;
    req["limit"] = (Json::UInt)limit;

    const auto addr = resolveSvcMessageAddr();
    auto rr = rockJsonRequest(addr, kCmdLoadHistoryRecords, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-message unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody())) {
        result.code = 500;
        result.err = "invalid svc-message response";
        return result;
    }
    IM::dto::MessagePage page;
    if (!out.isMember("data") || !parseMessagePage(out["data"], page)) {
        result.code = 500;
        result.err = "invalid message page";
        return result;
    }
    result.data = std::move(page);
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::MessageRecord>> MessageServiceRpcClient::LoadForwardRecords(
    const uint64_t current_user_id, const uint8_t talk_mode, const std::vector<std::string> &msg_ids) {
    Result<std::vector<IM::dto::MessageRecord>> result;

    Json::Value req(Json::objectValue);
    req["current_user_id"] = (Json::UInt64)current_user_id;
    req["talk_mode"] = (Json::UInt)talk_mode;
    Json::Value arr(Json::arrayValue);
    for (auto &s : msg_ids) arr.append(s);
    req["msg_ids"] = arr;

    const auto addr = resolveSvcMessageAddr();
    auto rr = rockJsonRequest(addr, kCmdLoadForwardRecords, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-message unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody())) {
        result.code = 500;
        result.err = "invalid svc-message response";
        return result;
    }
    if (!out.isMember("data") || !out["data"].isArray()) {
        result.code = 500;
        result.err = "invalid forward records";
        return result;
    }
    for (auto &it : out["data"]) {
        IM::dto::MessageRecord rec;
        if (parseMessageRecord(it, rec)) {
            result.data.push_back(std::move(rec));
        }
    }
    result.ok = true;
    return result;
}

Result<void> MessageServiceRpcClient::DeleteMessages(const uint64_t current_user_id, const uint8_t talk_mode,
                                                     const uint64_t to_from_id,
                                                     const std::vector<std::string> &msg_ids) {
    return callVoid(kCmdDeleteMessages, [&](Json::Value &req) {
        req["current_user_id"] = (Json::UInt64)current_user_id;
        req["talk_mode"] = (Json::UInt)talk_mode;
        req["to_from_id"] = (Json::UInt64)to_from_id;
        Json::Value arr(Json::arrayValue);
        for (auto &s : msg_ids) arr.append(s);
        req["msg_ids"] = arr;
    });
}

Result<void> MessageServiceRpcClient::DeleteAllMessagesInTalkForUser(const uint64_t current_user_id,
                                                                     const uint8_t talk_mode,
                                                                     const uint64_t to_from_id) {
    return callVoid(kCmdDeleteAllMessagesInTalkForUser, [&](Json::Value &req) {
        req["current_user_id"] = (Json::UInt64)current_user_id;
        req["talk_mode"] = (Json::UInt)talk_mode;
        req["to_from_id"] = (Json::UInt64)to_from_id;
    });
}

Result<void> MessageServiceRpcClient::ClearTalkRecords(const uint64_t current_user_id, const uint8_t talk_mode,
                                                       const uint64_t to_from_id) {
    return callVoid(kCmdClearTalkRecords, [&](Json::Value &req) {
        req["current_user_id"] = (Json::UInt64)current_user_id;
        req["talk_mode"] = (Json::UInt)talk_mode;
        req["to_from_id"] = (Json::UInt64)to_from_id;
    });
}

Result<void> MessageServiceRpcClient::RevokeMessage(const uint64_t current_user_id, const uint8_t talk_mode,
                                                    const uint64_t to_from_id, const std::string &msg_id) {
    return callVoid(kCmdRevokeMessage, [&](Json::Value &req) {
        req["current_user_id"] = (Json::UInt64)current_user_id;
        req["talk_mode"] = (Json::UInt)talk_mode;
        req["to_from_id"] = (Json::UInt64)to_from_id;
        req["msg_id"] = msg_id;
    });
}

Result<void> MessageServiceRpcClient::UpdateMessageStatus(const uint64_t current_user_id, const uint8_t talk_mode,
                                                          const uint64_t to_from_id, const std::string &msg_id,
                                                          uint8_t status) {
    return callVoid(kCmdUpdateMessageStatus, [&](Json::Value &req) {
        req["current_user_id"] = (Json::UInt64)current_user_id;
        req["talk_mode"] = (Json::UInt)talk_mode;
        req["to_from_id"] = (Json::UInt64)to_from_id;
        req["msg_id"] = msg_id;
        req["status"] = (Json::UInt)status;
    });
}

}  // namespace IM::app::rpc
