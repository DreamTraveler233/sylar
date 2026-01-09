#include "application/rpc/talk_service_rpc_client.hpp"

#include <utility>

#include "core/system/application.hpp"
#include "core/util/json_util.hpp"

namespace IM::app::rpc {

namespace {

constexpr uint32_t kTimeoutMs = 3000;

constexpr uint32_t kCmdGetSessionList = 701;
constexpr uint32_t kCmdSetSessionTop = 702;
constexpr uint32_t kCmdSetSessionDisturb = 703;
constexpr uint32_t kCmdCreateSession = 704;
constexpr uint32_t kCmdDeleteSession = 705;
constexpr uint32_t kCmdClearUnread = 706;

Result<void> FromRockVoid(const IM::RockResult::ptr& rr, const std::string& unavailable_msg) {
    Result<void> r;
    if (!rr || !rr->response) {
        r.code = 503;
        r.err = unavailable_msg;
        return r;
    }
    if (rr->response->getResult() != 200) {
        r.code = rr->response->getResult();
        r.err = rr->response->getResultStr();
        return r;
    }
    r.ok = true;
    return r;
}

}  // namespace

TalkServiceRpcClient::TalkServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("talk.rpc_addr", std::string(""),
                                   "svc-talk rpc address ip:port")) {}

IM::RockResult::ptr TalkServiceRpcClient::rockJsonRequest(const std::string& ip_port, uint32_t cmd,
                                                         const Json::Value& body,
                                                         uint32_t timeout_ms) {
    if (ip_port.empty()) {
        return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr,
                                                nullptr);
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
            return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr,
                                                    nullptr);
        }
        IM::RockConnection::ptr new_conn(new IM::RockConnection);
        if (!new_conn->connect(addr)) {
            return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr,
                                                    nullptr);
        }
        new_conn->start();
        {
            IM::RWMutex::WriteLock lock(m_mutex);
            m_conns[ip_port] = new_conn;
        }
        conn = std::move(new_conn);
    }

    IM::RockRequest::ptr req = std::make_shared<IM::RockRequest>();
    req->setSn(m_sn.fetch_add(1));
    req->setCmd(cmd);
    req->setBody(IM::JsonUtil::ToString(body));
    return conn->request(req, timeout_ms);
}

std::string TalkServiceRpcClient::resolveSvcTalkAddr() {
    const auto fixed = m_rpc_addr->getValue();
    if (!fixed.empty()) return fixed;

    if (auto sd = IM::Application::GetInstance()->getServiceDiscovery()) {
        std::unordered_map<
            std::string,
            std::unordered_map<std::string, std::unordered_map<uint64_t, IM::ServiceItemInfo::ptr>>>
            infos;
        sd->listServer(infos);
        auto itD = infos.find("im");
        if (itD == infos.end()) {
            sd->queryServer("im", "svc-talk");
            return {};
        }
        auto itS = itD->second.find("svc-talk");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-talk");
            return {};
        }
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }
    return {};
}

bool TalkServiceRpcClient::parseTalkSessionItem(const Json::Value& j, IM::dto::TalkSessionItem& out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetUint64(j, "id");
    out.talk_mode = IM::JsonUtil::GetUint8(j, "talk_mode");
    out.to_from_id = IM::JsonUtil::GetUint64(j, "to_from_id");
    out.is_top = IM::JsonUtil::GetUint8(j, "is_top");
    out.is_disturb = IM::JsonUtil::GetUint8(j, "is_disturb");
    out.is_robot = IM::JsonUtil::GetUint8(j, "is_robot");
    out.name = IM::JsonUtil::GetString(j, "name");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.remark = IM::JsonUtil::GetString(j, "remark");
    out.unread_num = IM::JsonUtil::GetUint32(j, "unread_num");
    out.msg_text = IM::JsonUtil::GetString(j, "msg_text");
    out.updated_at = IM::JsonUtil::GetString(j, "updated_at");
    return true;
}

Result<std::vector<IM::dto::TalkSessionItem>> TalkServiceRpcClient::getSessionListByUserId(
    const uint64_t user_id) {
    Result<std::vector<IM::dto::TalkSessionItem>> r;

    Json::Value body(Json::objectValue);
    body["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcTalkAddr(), kCmdGetSessionList, body, kTimeoutMs);
    if (!rr || !rr->response) {
        r.code = 503;
        r.err = "svc-talk unavailable";
        return r;
    }
    if (rr->response->getResult() != 200) {
        r.code = rr->response->getResult();
        r.err = rr->response->getResultStr();
        return r;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        r.code = 500;
        r.err = "invalid svc-talk response";
        return r;
    }

    const auto data = out["data"];
    const auto items = data["items"];
    if (items.isArray()) {
        r.data.reserve(items.size());
        for (const auto& it : items) {
            IM::dto::TalkSessionItem dto;
            if (parseTalkSessionItem(it, dto)) r.data.push_back(dto);
        }
    }

    r.ok = true;
    return r;
}

Result<void> TalkServiceRpcClient::setSessionTop(const uint64_t user_id, const uint64_t to_from_id,
                                                const uint8_t talk_mode,
                                                const uint8_t action) {
    Json::Value body(Json::objectValue);
    body["user_id"] = (Json::UInt64)user_id;
    body["to_from_id"] = (Json::UInt64)to_from_id;
    body["talk_mode"] = talk_mode;
    body["action"] = action;

    auto rr = rockJsonRequest(resolveSvcTalkAddr(), kCmdSetSessionTop, body, kTimeoutMs);
    return FromRockVoid(rr, "svc-talk unavailable");
}

Result<void> TalkServiceRpcClient::setSessionDisturb(const uint64_t user_id, const uint64_t to_from_id,
                                                    const uint8_t talk_mode,
                                                    const uint8_t action) {
    Json::Value body(Json::objectValue);
    body["user_id"] = (Json::UInt64)user_id;
    body["to_from_id"] = (Json::UInt64)to_from_id;
    body["talk_mode"] = talk_mode;
    body["action"] = action;

    auto rr = rockJsonRequest(resolveSvcTalkAddr(), kCmdSetSessionDisturb, body, kTimeoutMs);
    return FromRockVoid(rr, "svc-talk unavailable");
}

Result<IM::dto::TalkSessionItem> TalkServiceRpcClient::createSession(const uint64_t user_id,
                                                                     const uint64_t to_from_id,
                                                                     const uint8_t talk_mode) {
    Result<IM::dto::TalkSessionItem> r;

    Json::Value body(Json::objectValue);
    body["user_id"] = (Json::UInt64)user_id;
    body["to_from_id"] = (Json::UInt64)to_from_id;
    body["talk_mode"] = talk_mode;

    auto rr = rockJsonRequest(resolveSvcTalkAddr(), kCmdCreateSession, body, kTimeoutMs);
    if (!rr || !rr->response) {
        r.code = 503;
        r.err = "svc-talk unavailable";
        return r;
    }
    if (rr->response->getResult() != 200) {
        r.code = rr->response->getResult();
        r.err = rr->response->getResultStr();
        return r;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        r.code = 500;
        r.err = "invalid svc-talk response";
        return r;
    }

    if (!parseTalkSessionItem(out["data"], r.data)) {
        r.code = 500;
        r.err = "invalid talk session item";
        return r;
    }

    r.ok = true;
    return r;
}

Result<void> TalkServiceRpcClient::deleteSession(const uint64_t user_id, const uint64_t to_from_id,
                                                const uint8_t talk_mode) {
    Json::Value body(Json::objectValue);
    body["user_id"] = (Json::UInt64)user_id;
    body["to_from_id"] = (Json::UInt64)to_from_id;
    body["talk_mode"] = talk_mode;

    auto rr = rockJsonRequest(resolveSvcTalkAddr(), kCmdDeleteSession, body, kTimeoutMs);
    return FromRockVoid(rr, "svc-talk unavailable");
}

Result<void> TalkServiceRpcClient::clearSessionUnreadNum(const uint64_t user_id,
                                                        const uint64_t to_from_id,
                                                        const uint8_t talk_mode) {
    Json::Value body(Json::objectValue);
    body["user_id"] = (Json::UInt64)user_id;
    body["to_from_id"] = (Json::UInt64)to_from_id;
    body["talk_mode"] = talk_mode;

    auto rr = rockJsonRequest(resolveSvcTalkAddr(), kCmdClearUnread, body, kTimeoutMs);
    return FromRockVoid(rr, "svc-talk unavailable");
}

}  // namespace IM::app::rpc
