#include "application/rpc/contact_query_service_rpc_client.hpp"

#include "core/system/application.hpp"

namespace IM::app::rpc {

namespace {
constexpr uint32_t kCmdGetContactDetail = 401;
constexpr uint32_t kTimeoutMs = 3000;
}  // namespace

ContactQueryServiceRpcClient::ContactQueryServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("contact.rpc_addr", std::string(""), "svc-contact rpc address ip:port")) {}

IM::RockResult::ptr ContactQueryServiceRpcClient::rockJsonRequest(const std::string &ip_port, uint32_t cmd,
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

std::string ContactQueryServiceRpcClient::resolveSvcContactAddr() {
    const auto fixed = m_rpc_addr->getValue();
    if (!fixed.empty()) return fixed;

    if (auto sd = IM::Application::GetInstance()->getServiceDiscovery()) {
        std::unordered_map<std::string,
                           std::unordered_map<std::string, std::unordered_map<uint64_t, IM::ServiceItemInfo::ptr>>>
            infos;
        sd->listServer(infos);
        auto itD = infos.find("im");
        if (itD == infos.end()) {
            sd->queryServer("im", "svc-contact");
            return "";
        }
        auto itS = itD->second.find("svc-contact");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-contact");
            return "";
        }
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }
    return "";
}

bool ContactQueryServiceRpcClient::parseContactDetails(const Json::Value &j, IM::dto::ContactDetails &out) {
    if (!j.isObject()) return false;
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.gender = IM::JsonUtil::GetUint8(j, "gender");
    out.mobile = IM::JsonUtil::GetString(j, "mobile");
    out.motto = IM::JsonUtil::GetString(j, "motto");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.email = IM::JsonUtil::GetString(j, "email");
    out.relation = IM::JsonUtil::GetUint8(j, "relation");
    out.contact_group_id = IM::JsonUtil::GetUint32(j, "contact_group_id");
    out.contact_remark = IM::JsonUtil::GetString(j, "contact_remark");
    return out.user_id != 0;
}

Result<IM::dto::ContactDetails> ContactQueryServiceRpcClient::GetContactDetail(const uint64_t owner_id,
                                                                               const uint64_t target_id) {
    Result<IM::dto::ContactDetails> result;

    Json::Value req(Json::objectValue);
    req["owner_id"] = (Json::UInt64)owner_id;
    req["target_id"] = (Json::UInt64)target_id;

    const auto addr = resolveSvcContactAddr();
    auto rr = rockJsonRequest(addr, kCmdGetContactDetail, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-contact unavailable";
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
        result.err = "invalid svc-contact response";
        return result;
    }

    IM::dto::ContactDetails details;
    if (!out.isMember("data") || !parseContactDetails(out["data"], details)) {
        result.code = 500;
        result.err = "invalid contact details";
        return result;
    }

    result.data = std::move(details);
    result.ok = true;
    return result;
}

}  // namespace IM::app::rpc
