#include "application/rpc/contact_service_rpc_client.hpp"

#include "core/system/application.hpp"
#include "core/util/json_util.hpp"

namespace IM::app::rpc {

namespace {
constexpr uint32_t kTimeoutMs = 3000;

constexpr uint32_t kCmdGetContactDetail = 401;
constexpr uint32_t kCmdAgreeApply = 402;
constexpr uint32_t kCmdSearchByMobile = 403;
constexpr uint32_t kCmdListFriends = 404;
constexpr uint32_t kCmdCreateContactApply = 405;
constexpr uint32_t kCmdGetPendingContactApplyCount = 406;
constexpr uint32_t kCmdListContactApplies = 407;
constexpr uint32_t kCmdRejectApply = 408;
constexpr uint32_t kCmdEditContactRemark = 409;
constexpr uint32_t kCmdDeleteContact = 410;
constexpr uint32_t kCmdSaveContactGroup = 411;
constexpr uint32_t kCmdGetContactGroupLists = 412;
constexpr uint32_t kCmdChangeContactGroup = 413;

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

} // namespace

ContactServiceRpcClient::ContactServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("contact.rpc_addr", std::string(""),
                                   "svc-contact rpc address ip:port")) {}

IM::RockResult::ptr ContactServiceRpcClient::rockJsonRequest(const std::string& ip_port,
                                                             uint32_t cmd,
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

std::string ContactServiceRpcClient::resolveSvcContactAddr() {
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

bool ContactServiceRpcClient::parseTalkSession(const Json::Value& j, IM::dto::TalkSessionItem& out) {
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

bool ContactServiceRpcClient::parseUser(const Json::Value& j, IM::model::User& out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetUint64(j, "user_id");
    out.mobile = IM::JsonUtil::GetString(j, "mobile");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.gender = IM::JsonUtil::GetUint8(j, "gender");
    out.motto = IM::JsonUtil::GetString(j, "motto");
    return out.id != 0;
}

bool ContactServiceRpcClient::parseContactItem(const Json::Value& j, IM::dto::ContactItem& out) {
    if (!j.isObject()) return false;
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.gender = IM::JsonUtil::GetUint8(j, "gender");
    out.motto = IM::JsonUtil::GetString(j, "motto");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.remark = IM::JsonUtil::GetString(j, "remark");
    out.group_id = IM::JsonUtil::GetUint64(j, "group_id");
    return out.user_id != 0;
}

bool ContactServiceRpcClient::parseContactApplyItem(const Json::Value& j, IM::dto::ContactApplyItem& out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetUint64(j, "id");
    out.apply_user_id = IM::JsonUtil::GetUint64(j, "apply_user_id");
    out.target_user_id = IM::JsonUtil::GetUint64(j, "target_user_id");
    out.remark = IM::JsonUtil::GetString(j, "remark");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    return out.id != 0;
}

bool ContactServiceRpcClient::parseContactGroupItem(const Json::Value& j, IM::dto::ContactGroupItem& out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetUint64(j, "id");
    out.name = IM::JsonUtil::GetString(j, "name");
    out.contact_count = IM::JsonUtil::GetUint32(j, "count");
    out.sort = IM::JsonUtil::GetUint32(j, "sort");
    return out.id != 0;
}

bool ContactServiceRpcClient::parseContactDetails(const Json::Value& j, IM::dto::ContactDetails& out) {
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

Result<IM::dto::TalkSessionItem> ContactServiceRpcClient::AgreeApply(const uint64_t user_id,
                                                                    const uint64_t apply_id,
                                                                    const std::string& remark) {
    Result<IM::dto::TalkSessionItem> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["apply_id"] = (Json::UInt64)apply_id;
    req["remark"] = remark;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdAgreeApply, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    IM::dto::TalkSessionItem s;
    if (!out.isMember("data") || !parseTalkSession(out["data"], s)) {
        result.code = 500;
        result.err = "invalid session";
        return result;
    }

    result.data = std::move(s);
    result.ok = true;
    return result;
}

Result<IM::model::User> ContactServiceRpcClient::SearchByMobile(const std::string& mobile) {
    Result<IM::model::User> result;

    Json::Value req(Json::objectValue);
    req["mobile"] = mobile;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdSearchByMobile, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    IM::model::User u;
    if (!out.isMember("data") || !parseUser(out["data"], u)) {
        result.code = 500;
        result.err = "invalid user";
        return result;
    }

    result.data = std::move(u);
    result.ok = true;
    return result;
}

Result<IM::dto::ContactDetails> ContactServiceRpcClient::GetContactDetail(const uint64_t user_id,
                                                                          const uint64_t target_id) {
    Result<IM::dto::ContactDetails> result;

    Json::Value req(Json::objectValue);
    req["owner_id"] = (Json::UInt64)user_id;
    req["target_id"] = (Json::UInt64)target_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdGetContactDetail, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    IM::dto::ContactDetails d;
    if (!out.isMember("data") || !parseContactDetails(out["data"], d)) {
        result.code = 500;
        result.err = "invalid contact details";
        return result;
    }

    result.data = std::move(d);
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::ContactItem>> ContactServiceRpcClient::ListFriends(const uint64_t user_id) {
    Result<std::vector<IM::dto::ContactItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdListFriends, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid contacts";
        return result;
    }

    std::vector<IM::dto::ContactItem> items;
    for (const auto& it : out["data"]["items"]) {
        IM::dto::ContactItem c;
        if (parseContactItem(it, c)) items.push_back(std::move(c));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<void> ContactServiceRpcClient::CreateContactApply(const uint64_t apply_user_id,
                                                         const uint64_t target_user_id,
                                                         const std::string& remark) {
    Json::Value req(Json::objectValue);
    req["apply_user_id"] = (Json::UInt64)apply_user_id;
    req["target_user_id"] = (Json::UInt64)target_user_id;
    req["remark"] = remark;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdCreateContactApply, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-contact unavailable");
}

Result<uint64_t> ContactServiceRpcClient::GetPendingContactApplyCount(const uint64_t user_id) {
    Result<uint64_t> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdGetPendingContactApplyCount, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject()) {
        result.code = 500;
        result.err = "invalid num";
        return result;
    }

    result.data = IM::JsonUtil::GetUint64(out["data"], "num");
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::ContactApplyItem>> ContactServiceRpcClient::ListContactApplies(const uint64_t user_id) {
    Result<std::vector<IM::dto::ContactApplyItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdListContactApplies, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid apply list";
        return result;
    }

    std::vector<IM::dto::ContactApplyItem> items;
    for (const auto& it : out["data"]["items"]) {
        IM::dto::ContactApplyItem a;
        if (parseContactApplyItem(it, a)) items.push_back(std::move(a));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<void> ContactServiceRpcClient::RejectApply(const uint64_t handler_user_id,
                                                  const uint64_t apply_user_id,
                                                  const std::string& remark) {
    Json::Value req(Json::objectValue);
    req["handler_user_id"] = (Json::UInt64)handler_user_id;
    req["apply_user_id"] = (Json::UInt64)apply_user_id;
    req["remark"] = remark;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdRejectApply, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-contact unavailable");
}

Result<void> ContactServiceRpcClient::EditContactRemark(const uint64_t user_id,
                                                        const uint64_t contact_id,
                                                        const std::string& remark) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["contact_id"] = (Json::UInt64)contact_id;
    req["remark"] = remark;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdEditContactRemark, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-contact unavailable");
}

Result<void> ContactServiceRpcClient::DeleteContact(const uint64_t user_id, const uint64_t contact_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["contact_id"] = (Json::UInt64)contact_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdDeleteContact, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-contact unavailable");
}

Result<void> ContactServiceRpcClient::SaveContactGroup(
    const uint64_t user_id,
    const std::vector<std::tuple<uint64_t, uint64_t, std::string>>& groupItems) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    Json::Value items(Json::arrayValue);
    for (const auto& it : groupItems) {
        Json::Value j(Json::objectValue);
        j["id"] = (Json::UInt64)std::get<0>(it);
        j["sort"] = (Json::UInt64)std::get<1>(it);
        j["name"] = std::get<2>(it);
        items.append(j);
    }
    req["items"] = items;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdSaveContactGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-contact unavailable");
}

Result<std::vector<IM::dto::ContactGroupItem>> ContactServiceRpcClient::GetContactGroupLists(const uint64_t user_id) {
    Result<std::vector<IM::dto::ContactGroupItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdGetContactGroupLists, req, kTimeoutMs);
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
    if (!IM::JsonUtil::FromString(out, rr->response->getBody()) || !out.isObject()) {
        result.code = 500;
        result.err = "invalid svc-contact response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid group list";
        return result;
    }

    std::vector<IM::dto::ContactGroupItem> items;
    for (const auto& it : out["data"]["items"]) {
        IM::dto::ContactGroupItem g;
        if (parseContactGroupItem(it, g)) items.push_back(std::move(g));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<void> ContactServiceRpcClient::ChangeContactGroup(const uint64_t user_id,
                                                         const uint64_t contact_id,
                                                         const uint64_t group_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["contact_id"] = (Json::UInt64)contact_id;
    req["group_id"] = (Json::UInt64)group_id;

    auto rr = rockJsonRequest(resolveSvcContactAddr(), kCmdChangeContactGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-contact unavailable");
}

} // namespace IM::app::rpc
