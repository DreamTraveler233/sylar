#include "application/rpc/group_service_rpc_client.hpp"

#include "core/system/application.hpp"
#include "core/util/json_util.hpp"

namespace IM::app::rpc {

namespace {
constexpr uint32_t kTimeoutMs = 3000;

constexpr uint32_t kCmdCreateGroup = 601;
constexpr uint32_t kCmdDismissGroup = 602;
constexpr uint32_t kCmdGetGroupDetail = 603;
constexpr uint32_t kCmdGetGroupList = 604;
constexpr uint32_t kCmdUpdateGroupSetting = 605;
constexpr uint32_t kCmdHandoverGroup = 606;
constexpr uint32_t kCmdAssignAdmin = 607;
constexpr uint32_t kCmdMuteGroup = 608;
constexpr uint32_t kCmdOvertGroup = 609;
constexpr uint32_t kCmdGetOvertGroupList = 610;

constexpr uint32_t kCmdGetGroupMemberList = 611;
constexpr uint32_t kCmdInviteGroup = 612;
constexpr uint32_t kCmdRemoveMember = 613;
constexpr uint32_t kCmdSecedeGroup = 614;
constexpr uint32_t kCmdUpdateMemberRemark = 615;
constexpr uint32_t kCmdMuteMember = 616;

constexpr uint32_t kCmdCreateApply = 617;
constexpr uint32_t kCmdAgreeApply = 618;
constexpr uint32_t kCmdDeclineApply = 619;
constexpr uint32_t kCmdGetApplyList = 620;
constexpr uint32_t kCmdGetUserApplyList = 621;
constexpr uint32_t kCmdGetUnreadApplyCount = 622;

constexpr uint32_t kCmdEditNotice = 623;

constexpr uint32_t kCmdCreateVote = 624;
constexpr uint32_t kCmdGetVoteList = 625;
constexpr uint32_t kCmdGetVoteDetail = 626;
constexpr uint32_t kCmdCastVote = 627;
constexpr uint32_t kCmdFinishVote = 628;

Result<void> FromRockVoid(const IM::RockResult::ptr &rr, const std::string &unavailable_msg) {
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

GroupServiceRpcClient::GroupServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("group.rpc_addr", std::string(""), "svc-group rpc address ip:port")) {}

IM::RockResult::ptr GroupServiceRpcClient::rockJsonRequest(const std::string &ip_port, uint32_t cmd,
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

std::string GroupServiceRpcClient::resolveSvcGroupAddr() {
    const auto fixed = m_rpc_addr->getValue();
    if (!fixed.empty()) return fixed;

    if (auto sd = IM::Application::GetInstance()->getServiceDiscovery()) {
        std::unordered_map<std::string,
                           std::unordered_map<std::string, std::unordered_map<uint64_t, IM::ServiceItemInfo::ptr>>>
            infos;
        sd->listServer(infos);
        auto itD = infos.find("im");
        if (itD == infos.end()) {
            sd->queryServer("im", "svc-group");
            return "";
        }
        auto itS = itD->second.find("svc-group");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-group");
            return "";
        }
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }
    return "";
}

bool GroupServiceRpcClient::parseGroupItem(const Json::Value &j, IM::dto::GroupItem &out) {
    if (!j.isObject()) return false;
    out.group_id = IM::JsonUtil::GetUint64(j, "group_id");
    out.group_name = IM::JsonUtil::GetString(j, "group_name");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.profile = IM::JsonUtil::GetString(j, "profile");
    out.leader = IM::JsonUtil::GetUint64(j, "leader");
    out.creator_id = IM::JsonUtil::GetUint64(j, "creator_id");
    return out.group_id != 0;
}

bool GroupServiceRpcClient::parseGroupDetail(const Json::Value &j, IM::dto::GroupDetail &out) {
    if (!j.isObject()) return false;
    out.group_id = IM::JsonUtil::GetUint64(j, "group_id");
    out.group_name = IM::JsonUtil::GetString(j, "group_name");
    out.profile = IM::JsonUtil::GetString(j, "profile");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    out.is_manager = j.isMember("is_manager") ? j["is_manager"].asBool() : false;
    out.is_disturb = IM::JsonUtil::GetInt32(j, "is_disturb");
    out.visit_card = IM::JsonUtil::GetString(j, "visit_card");
    out.is_mute = IM::JsonUtil::GetInt32(j, "is_mute");
    out.is_overt = IM::JsonUtil::GetInt32(j, "is_overt");
    if (j.isMember("notice") && j["notice"].isObject()) {
        const auto &n = j["notice"];
        out.notice.content = IM::JsonUtil::GetString(n, "content");
        out.notice.created_at = IM::JsonUtil::GetString(n, "created_at");
        out.notice.updated_at = IM::JsonUtil::GetString(n, "updated_at");
        out.notice.modify_user_name = IM::JsonUtil::GetString(n, "modify_user_name");
    }
    return out.group_id != 0;
}

bool GroupServiceRpcClient::parseGroupMemberItem(const Json::Value &j, IM::dto::GroupMemberItem &out) {
    if (!j.isObject()) return false;
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.gender = IM::JsonUtil::GetInt32(j, "gender");
    out.leader = IM::JsonUtil::GetInt32(j, "leader");
    out.is_mute = IM::JsonUtil::GetInt32(j, "is_mute");
    out.remark = IM::JsonUtil::GetString(j, "remark");
    out.motto = IM::JsonUtil::GetString(j, "motto");
    out.visit_card = IM::JsonUtil::GetString(j, "visit_card");
    return out.user_id != 0;
}

bool GroupServiceRpcClient::parseGroupApplyItem(const Json::Value &j, IM::dto::GroupApplyItem &out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetUint64(j, "id");
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.group_id = IM::JsonUtil::GetUint64(j, "group_id");
    out.remark = IM::JsonUtil::GetString(j, "remark");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    out.group_name = IM::JsonUtil::GetString(j, "group_name");
    return out.id != 0;
}

bool GroupServiceRpcClient::parseGroupOvertItem(const Json::Value &j, IM::dto::GroupOvertItem &out) {
    if (!j.isObject()) return false;
    out.group_id = IM::JsonUtil::GetUint64(j, "group_id");
    out.type = IM::JsonUtil::GetInt32(j, "type");
    out.name = IM::JsonUtil::GetString(j, "name");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.profile = IM::JsonUtil::GetString(j, "profile");
    out.count = IM::JsonUtil::GetInt32(j, "count");
    out.max_num = IM::JsonUtil::GetInt32(j, "max_num");
    out.is_member = j.isMember("is_member") ? j["is_member"].asBool() : false;
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    return out.group_id != 0;
}

bool GroupServiceRpcClient::parseGroupVoteItem(const Json::Value &j, IM::dto::GroupVoteItem &out) {
    if (!j.isObject()) return false;
    out.vote_id = IM::JsonUtil::GetUint64(j, "vote_id");
    out.title = IM::JsonUtil::GetString(j, "title");
    out.answer_mode = IM::JsonUtil::GetInt32(j, "answer_mode");
    out.is_anonymous = IM::JsonUtil::GetInt32(j, "is_anonymous");
    out.status = IM::JsonUtil::GetInt32(j, "status");
    out.created_by = IM::JsonUtil::GetUint64(j, "created_by");
    out.is_voted = j.isMember("is_voted") ? j["is_voted"].asBool() : false;
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    return out.vote_id != 0;
}

bool GroupServiceRpcClient::parseGroupVoteDetail(const Json::Value &j, IM::dto::GroupVoteDetail &out) {
    if (!j.isObject()) return false;
    out.vote_id = IM::JsonUtil::GetUint64(j, "vote_id");
    out.title = IM::JsonUtil::GetString(j, "title");
    out.answer_mode = IM::JsonUtil::GetInt32(j, "answer_mode");
    out.is_anonymous = IM::JsonUtil::GetInt32(j, "is_anonymous");
    out.status = IM::JsonUtil::GetInt32(j, "status");
    out.created_by = IM::JsonUtil::GetUint64(j, "created_by");
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    out.voted_count = IM::JsonUtil::GetInt32(j, "voted_count");
    out.is_voted = j.isMember("is_voted") ? j["is_voted"].asBool() : false;

    out.options.clear();
    if (j.isMember("options") && j["options"].isArray()) {
        for (const auto &it : j["options"]) {
            if (!it.isObject()) continue;
            IM::dto::GroupVoteOptionItem opt;
            opt.id = IM::JsonUtil::GetUint64(it, "id");
            opt.content = IM::JsonUtil::GetString(it, "content");
            opt.count = IM::JsonUtil::GetInt32(it, "count");
            opt.is_voted = it.isMember("is_voted") ? it["is_voted"].asBool() : false;
            if (it.isMember("users") && it["users"].isArray()) {
                for (const auto &u : it["users"]) {
                    if (u.isString()) opt.users.push_back(u.asString());
                }
            }
            out.options.push_back(std::move(opt));
        }
    }

    return out.vote_id != 0;
}

// ---- Methods ----

Result<uint64_t> GroupServiceRpcClient::CreateGroup(uint64_t user_id, const std::string &name,
                                                    const std::vector<uint64_t> &member_ids) {
    Result<uint64_t> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["name"] = name;
    Json::Value arr(Json::arrayValue);
    for (auto id : member_ids) arr.append((Json::UInt64)id);
    // HTTP API 使用 user_ids，这里也走 user_ids
    req["user_ids"] = arr;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdCreateGroup, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }
    if (!out.isMember("data") || !out["data"].isObject()) {
        result.code = 500;
        result.err = "invalid data";
        return result;
    }

    result.data = IM::JsonUtil::GetUint64(out["data"], "group_id");
    result.ok = true;
    return result;
}

Result<void> GroupServiceRpcClient::DismissGroup(uint64_t user_id, uint64_t group_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdDismissGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<IM::dto::GroupDetail> GroupServiceRpcClient::GetGroupDetail(uint64_t user_id, uint64_t group_id) {
    Result<IM::dto::GroupDetail> result;
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetGroupDetail, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    IM::dto::GroupDetail d;
    if (!out.isMember("data") || !parseGroupDetail(out["data"], d)) {
        result.code = 500;
        result.err = "invalid group detail";
        return result;
    }

    result.data = std::move(d);
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::GroupItem>> GroupServiceRpcClient::GetGroupList(uint64_t user_id) {
    Result<std::vector<IM::dto::GroupItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetGroupList, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid group list";
        return result;
    }

    std::vector<IM::dto::GroupItem> items;
    for (const auto &it : out["data"]["items"]) {
        IM::dto::GroupItem g;
        if (parseGroupItem(it, g)) items.push_back(std::move(g));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<void> GroupServiceRpcClient::UpdateGroupSetting(uint64_t user_id, uint64_t group_id, const std::string &name,
                                                       const std::string &avatar, const std::string &profile) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["name"] = name;
    req["avatar"] = avatar;
    req["profile"] = profile;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdUpdateGroupSetting, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::HandoverGroup(uint64_t user_id, uint64_t group_id, uint64_t new_owner_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["new_owner_id"] = (Json::UInt64)new_owner_id;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdHandoverGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::AssignAdmin(uint64_t user_id, uint64_t group_id, uint64_t target_id, int action) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["target_id"] = (Json::UInt64)target_id;
    req["action"] = action;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdAssignAdmin, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::MuteGroup(uint64_t user_id, uint64_t group_id, int action) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["action"] = action;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdMuteGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::OvertGroup(uint64_t user_id, uint64_t group_id, int action) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["action"] = action;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdOvertGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<std::pair<std::vector<IM::dto::GroupOvertItem>, bool>> GroupServiceRpcClient::GetOvertGroupList(
    int page, const std::string &name) {
    Result<std::pair<std::vector<IM::dto::GroupOvertItem>, bool>> result;

    Json::Value req(Json::objectValue);
    req["page"] = page;
    req["name"] = name;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetOvertGroupList, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid overt list";
        return result;
    }

    std::vector<IM::dto::GroupOvertItem> items;
    for (const auto &it : out["data"]["items"]) {
        IM::dto::GroupOvertItem g;
        if (parseGroupOvertItem(it, g)) items.push_back(std::move(g));
    }

    const bool has_more = out["data"].isMember("has_more") ? out["data"]["has_more"].asBool() : false;

    result.data = std::make_pair(std::move(items), has_more);
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::GroupMemberItem>> GroupServiceRpcClient::GetGroupMemberList(uint64_t user_id,
                                                                                        uint64_t group_id) {
    Result<std::vector<IM::dto::GroupMemberItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetGroupMemberList, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid members";
        return result;
    }

    std::vector<IM::dto::GroupMemberItem> items;
    for (const auto &it : out["data"]["items"]) {
        IM::dto::GroupMemberItem m;
        if (parseGroupMemberItem(it, m)) items.push_back(std::move(m));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<void> GroupServiceRpcClient::InviteGroup(uint64_t user_id, uint64_t group_id,
                                                const std::vector<uint64_t> &member_ids) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    Json::Value arr(Json::arrayValue);
    for (auto id : member_ids) arr.append((Json::UInt64)id);
    req["user_ids"] = arr;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdInviteGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::RemoveMember(uint64_t user_id, uint64_t group_id,
                                                 const std::vector<uint64_t> &member_ids) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    Json::Value arr(Json::arrayValue);
    for (auto id : member_ids) arr.append((Json::UInt64)id);
    req["user_ids"] = arr;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdRemoveMember, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::SecedeGroup(uint64_t user_id, uint64_t group_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdSecedeGroup, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::UpdateMemberRemark(uint64_t user_id, uint64_t group_id, const std::string &remark) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["remark"] = remark;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdUpdateMemberRemark, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::MuteMember(uint64_t user_id, uint64_t group_id, uint64_t target_id, int action) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["target_id"] = (Json::UInt64)target_id;
    req["action"] = action;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdMuteMember, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::CreateApply(uint64_t user_id, uint64_t group_id, const std::string &remark) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["remark"] = remark;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdCreateApply, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::AgreeApply(uint64_t user_id, uint64_t apply_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["apply_id"] = (Json::UInt64)apply_id;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdAgreeApply, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::DeclineApply(uint64_t user_id, uint64_t apply_id, const std::string &remark) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["apply_id"] = (Json::UInt64)apply_id;
    req["remark"] = remark;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdDeclineApply, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<std::vector<IM::dto::GroupApplyItem>> GroupServiceRpcClient::GetApplyList(uint64_t user_id, uint64_t group_id) {
    Result<std::vector<IM::dto::GroupApplyItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetApplyList, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid apply list";
        return result;
    }

    std::vector<IM::dto::GroupApplyItem> items;
    for (const auto &it : out["data"]["items"]) {
        IM::dto::GroupApplyItem a;
        if (parseGroupApplyItem(it, a)) items.push_back(std::move(a));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::GroupApplyItem>> GroupServiceRpcClient::GetUserApplyList(uint64_t user_id) {
    Result<std::vector<IM::dto::GroupApplyItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetUserApplyList, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid apply list";
        return result;
    }

    std::vector<IM::dto::GroupApplyItem> items;
    for (const auto &it : out["data"]["items"]) {
        IM::dto::GroupApplyItem a;
        if (parseGroupApplyItem(it, a)) items.push_back(std::move(a));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<int> GroupServiceRpcClient::GetUnreadApplyCount(uint64_t user_id) {
    Result<int> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetUnreadApplyCount, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject()) {
        result.code = 500;
        result.err = "invalid num";
        return result;
    }

    result.data = IM::JsonUtil::GetInt32(out["data"], "num");
    result.ok = true;
    return result;
}

Result<void> GroupServiceRpcClient::EditNotice(uint64_t user_id, uint64_t group_id, const std::string &content) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["content"] = content;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdEditNotice, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<uint64_t> GroupServiceRpcClient::CreateVote(uint64_t user_id, uint64_t group_id, const std::string &title,
                                                   int answer_mode, int is_anonymous,
                                                   const std::vector<std::string> &options) {
    Result<uint64_t> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;
    req["title"] = title;
    req["answer_mode"] = answer_mode;
    req["is_anonymous"] = is_anonymous;
    Json::Value arr(Json::arrayValue);
    for (const auto &o : options) arr.append(o);
    req["options"] = arr;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdCreateVote, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject()) {
        result.code = 500;
        result.err = "invalid data";
        return result;
    }

    result.data = IM::JsonUtil::GetUint64(out["data"], "vote_id");
    result.ok = true;
    return result;
}

Result<std::vector<IM::dto::GroupVoteItem>> GroupServiceRpcClient::GetVoteList(uint64_t user_id, uint64_t group_id) {
    Result<std::vector<IM::dto::GroupVoteItem>> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["group_id"] = (Json::UInt64)group_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetVoteList, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    if (!out.isMember("data") || !out["data"].isObject() || !out["data"].isMember("items") ||
        !out["data"]["items"].isArray()) {
        result.code = 500;
        result.err = "invalid vote list";
        return result;
    }

    std::vector<IM::dto::GroupVoteItem> items;
    for (const auto &it : out["data"]["items"]) {
        IM::dto::GroupVoteItem v;
        if (parseGroupVoteItem(it, v)) items.push_back(std::move(v));
    }

    result.data = std::move(items);
    result.ok = true;
    return result;
}

Result<IM::dto::GroupVoteDetail> GroupServiceRpcClient::GetVoteDetail(uint64_t user_id, uint64_t vote_id) {
    Result<IM::dto::GroupVoteDetail> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["vote_id"] = (Json::UInt64)vote_id;

    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdGetVoteDetail, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-group unavailable";
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
        result.err = "invalid svc-group response";
        return result;
    }

    IM::dto::GroupVoteDetail d;
    if (!out.isMember("data") || !parseGroupVoteDetail(out["data"], d)) {
        result.code = 500;
        result.err = "invalid vote detail";
        return result;
    }

    result.data = std::move(d);
    result.ok = true;
    return result;
}

Result<void> GroupServiceRpcClient::CastVote(uint64_t user_id, uint64_t vote_id,
                                             const std::vector<std::string> &options) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["vote_id"] = (Json::UInt64)vote_id;
    Json::Value arr(Json::arrayValue);
    for (const auto &o : options) arr.append(o);
    req["options"] = arr;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdCastVote, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

Result<void> GroupServiceRpcClient::FinishVote(uint64_t user_id, uint64_t vote_id) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["vote_id"] = (Json::UInt64)vote_id;
    auto rr = rockJsonRequest(resolveSvcGroupAddr(), kCmdFinishVote, req, kTimeoutMs);
    return FromRockVoid(rr, "svc-group unavailable");
}

}  // namespace IM::app::rpc
