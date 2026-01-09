#include "application/rpc/talk_repository_rpc_client.hpp"

#include "core/base/macro.hpp"
#include "core/system/application.hpp"
#include "core/util/json_util.hpp"

namespace IM::app::rpc {

static auto g_logger = IM_LOG_NAME("root");

// NOTE:
// Config system only applies YAML values to *pre-registered* variables.
// We register talk.rpc_addr at static-init time to ensure services like svc_message
// (which may construct TalkRepositoryRpcClient later during runtime) can still
// read the configured fixed address.
static auto g_talk_rpc_addr =
    IM::Config::Lookup("talk.rpc_addr", std::string(""), "svc-talk rpc address ip:port");

namespace {

constexpr uint32_t kTimeoutMs = 800;

// talk query cmd allocation (ws needs)
constexpr uint32_t kCmdGetGroupTalkId = 707;
constexpr uint32_t kCmdListUsersByTalkId = 708;

}  // namespace

TalkRepositoryRpcClient::TalkRepositoryRpcClient()
    : m_rpc_addr(g_talk_rpc_addr) {}

IM::RockResult::ptr TalkRepositoryRpcClient::rockJsonRequest(const std::string& ip_port, uint32_t cmd,
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
        conn = new_conn;
    }

    IM::RockRequest::ptr req = std::make_shared<IM::RockRequest>();
    req->setSn(m_sn.fetch_add(1));
    req->setCmd(cmd);
    req->setBody(IM::JsonUtil::ToString(body));
    return conn->request(req, timeout_ms);
}

std::string TalkRepositoryRpcClient::resolveSvcTalkAddr() {
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
            return "";
        }
        auto itS = itD->second.find("svc-talk");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-talk");
            return "";
        }
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }
    return "";
}

bool TalkRepositoryRpcClient::getGroupTalkId(const uint64_t group_id, uint64_t& out_talk_id,
                                            std::string* err) {
    Json::Value req(Json::objectValue);
    req["group_id"] = (Json::UInt64)group_id;

    const auto addr = resolveSvcTalkAddr();
    IM_LOG_INFO(g_logger) << "TalkRepoRpc getGroupTalkId -> svc-talk addr='" << addr
                          << "' group_id=" << group_id;

    auto rr = rockJsonRequest(addr, kCmdGetGroupTalkId, req, kTimeoutMs);
    if (!rr || !rr->response) {
        if (err) *err = "svc-talk unavailable";
        IM_LOG_WARN(g_logger) << "TalkRepoRpc getGroupTalkId failed: no response";
        return false;
    }
    if (rr->response->getResult() != 200) {
        if (err) *err = rr->response->getResultStr();
        IM_LOG_WARN(g_logger) << "TalkRepoRpc getGroupTalkId failed: result="
                              << rr->response->getResult() << " msg='"
                              << rr->response->getResultStr() << "'";
        return false;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        if (err) *err = "invalid svc-talk response";
        return false;
    }

    out_talk_id = IM::JsonUtil::GetUint64(out["data"], "talk_id");
    if (out_talk_id == 0) {
        if (err) *err = "talk_id not found";
        IM_LOG_WARN(g_logger) << "TalkRepoRpc getGroupTalkId failed: talk_id=0";
        return false;
    }
    IM_LOG_INFO(g_logger) << "TalkRepoRpc getGroupTalkId ok: talk_id=" << out_talk_id;
    return true;
}

bool TalkRepositoryRpcClient::listUsersByTalkId(const uint64_t talk_id,
                                               std::vector<uint64_t>& out_user_ids,
                                               std::string* err) {
    Json::Value req(Json::objectValue);
    req["talk_id"] = (Json::UInt64)talk_id;

    const auto addr = resolveSvcTalkAddr();
    IM_LOG_INFO(g_logger) << "TalkRepoRpc listUsersByTalkId -> svc-talk addr='" << addr
                          << "' talk_id=" << talk_id;

    auto rr = rockJsonRequest(addr, kCmdListUsersByTalkId, req, kTimeoutMs);
    if (!rr || !rr->response) {
        if (err) *err = "svc-talk unavailable";
        IM_LOG_WARN(g_logger) << "TalkRepoRpc listUsersByTalkId failed: no response";
        return false;
    }
    if (rr->response->getResult() != 200) {
        if (err) *err = rr->response->getResultStr();
        IM_LOG_WARN(g_logger) << "TalkRepoRpc listUsersByTalkId failed: result="
                              << rr->response->getResult() << " msg='"
                              << rr->response->getResultStr() << "'";
        return false;
    }

    Json::Value out;
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        if (err) *err = "invalid svc-talk response";
        return false;
    }

    out_user_ids.clear();
    const auto arr = out["data"]["user_ids"];
    if (arr.isArray()) {
        out_user_ids.reserve(arr.size());
        for (const auto& it : arr) {
            if (it.isUInt64()) {
                out_user_ids.push_back((uint64_t)it.asUInt64());
            } else if (it.isString()) {
                try {
                    out_user_ids.push_back((uint64_t)std::stoull(it.asString()));
                } catch (...) {
                }
            }
        }
    }
    IM_LOG_INFO(g_logger) << "TalkRepoRpc listUsersByTalkId ok: users=" << out_user_ids.size();
    return true;
}

}  // namespace IM::app::rpc
