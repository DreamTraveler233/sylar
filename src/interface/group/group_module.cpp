#include "interface/group/group_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/util/json_util.hpp"

namespace IM::group {

static auto g_logger = IM_LOG_NAME("root");

namespace {
constexpr uint32_t kTimeoutMs = 5000;

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

static void WriteErr(IM::RockResponse::ptr response, uint32_t code, const std::string &err) {
    response->setResult(code);
    response->setResultStr(err);
}

static void WriteOk(IM::RockResponse::ptr response, const Json::Value &data) {
    Json::Value out(Json::objectValue);
    out["data"] = data;
    response->setBody(IM::JsonUtil::ToString(out));
    response->setResult(200);
    response->setResultStr("ok");
}

static Json::Value GroupItemToJson(const IM::dto::GroupItem &it) {
    Json::Value j(Json::objectValue);
    j["group_id"] = (Json::UInt64)it.group_id;
    j["group_name"] = it.group_name;
    j["avatar"] = it.avatar;
    j["profile"] = it.profile;
    j["leader"] = (Json::UInt64)it.leader;
    j["creator_id"] = (Json::UInt64)it.creator_id;
    return j;
}

static Json::Value GroupMemberItemToJson(const IM::dto::GroupMemberItem &it) {
    Json::Value j(Json::objectValue);
    j["user_id"] = (Json::UInt64)it.user_id;
    j["nickname"] = it.nickname;
    j["avatar"] = it.avatar;
    j["gender"] = it.gender;
    j["leader"] = it.leader;
    j["is_mute"] = it.is_mute;
    j["remark"] = it.remark;
    j["motto"] = it.motto;
    j["visit_card"] = it.visit_card;
    return j;
}

static Json::Value GroupApplyItemToJson(const IM::dto::GroupApplyItem &it) {
    Json::Value j(Json::objectValue);
    j["id"] = (Json::UInt64)it.id;
    j["user_id"] = (Json::UInt64)it.user_id;
    j["group_id"] = (Json::UInt64)it.group_id;
    j["remark"] = it.remark;
    j["avatar"] = it.avatar;
    j["nickname"] = it.nickname;
    j["created_at"] = it.created_at;
    j["group_name"] = it.group_name;
    return j;
}

static Json::Value GroupOvertItemToJson(const IM::dto::GroupOvertItem &it) {
    Json::Value j(Json::objectValue);
    j["group_id"] = (Json::UInt64)it.group_id;
    j["type"] = it.type;
    j["name"] = it.name;
    j["avatar"] = it.avatar;
    j["profile"] = it.profile;
    j["count"] = it.count;
    j["max_num"] = it.max_num;
    j["is_member"] = it.is_member;
    j["created_at"] = it.created_at;
    return j;
}

static Json::Value GroupDetailToJson(const IM::dto::GroupDetail &d) {
    Json::Value j(Json::objectValue);
    j["group_id"] = (Json::UInt64)d.group_id;
    j["group_name"] = d.group_name;
    j["profile"] = d.profile;
    j["avatar"] = d.avatar;
    j["created_at"] = d.created_at;
    j["is_manager"] = d.is_manager;
    j["is_disturb"] = d.is_disturb;
    j["visit_card"] = d.visit_card;
    j["is_mute"] = d.is_mute;
    j["is_overt"] = d.is_overt;

    Json::Value n(Json::objectValue);
    n["content"] = d.notice.content;
    n["created_at"] = d.notice.created_at;
    n["updated_at"] = d.notice.updated_at;
    n["modify_user_name"] = d.notice.modify_user_name;
    j["notice"] = n;

    return j;
}

static Json::Value GroupVoteItemToJson(const IM::dto::GroupVoteItem &it) {
    Json::Value j(Json::objectValue);
    j["vote_id"] = (Json::UInt64)it.vote_id;
    j["title"] = it.title;
    j["answer_mode"] = it.answer_mode;
    j["is_anonymous"] = it.is_anonymous;
    j["status"] = it.status;
    j["created_by"] = (Json::UInt64)it.created_by;
    j["is_voted"] = it.is_voted;
    j["created_at"] = it.created_at;
    return j;
}

static Json::Value GroupVoteDetailToJson(const IM::dto::GroupVoteDetail &d) {
    Json::Value j(Json::objectValue);
    j["vote_id"] = (Json::UInt64)d.vote_id;
    j["title"] = d.title;
    j["answer_mode"] = d.answer_mode;
    j["is_anonymous"] = d.is_anonymous;
    j["status"] = d.status;
    j["created_by"] = (Json::UInt64)d.created_by;
    j["created_at"] = d.created_at;
    j["voted_count"] = d.voted_count;
    j["is_voted"] = d.is_voted;

    Json::Value opts(Json::arrayValue);
    for (const auto &opt : d.options) {
        Json::Value o(Json::objectValue);
        o["id"] = (Json::UInt64)opt.id;
        o["content"] = opt.content;
        o["count"] = opt.count;
        o["is_voted"] = opt.is_voted;
        Json::Value users(Json::arrayValue);
        for (const auto &u : opt.users) {
            users.append(u);
        }
        o["users"] = users;
        opts.append(o);
    }
    j["options"] = opts;
    return j;
}

static std::vector<uint64_t> ParseUint64Array(const Json::Value &arr) {
    std::vector<uint64_t> out;
    if (!arr.isArray()) return out;
    out.reserve(arr.size());
    for (const auto &v : arr) {
        if (v.isUInt64())
            out.push_back(v.asUInt64());
        else if (v.isString()) {
            try {
                out.push_back(std::stoull(v.asString()));
            } catch (...) {
            }
        }
    }
    return out;
}

static std::vector<uint64_t> ParseUint64ArrayAlias(const Json::Value &obj, const char *k1, const char *k2) {
    if (obj.isMember(k1)) {
        auto v = ParseUint64Array(obj[k1]);
        if (!v.empty()) return v;
    }
    if (obj.isMember(k2)) {
        return ParseUint64Array(obj[k2]);
    }
    return {};
}

static std::vector<std::string> ParseStringArray(const Json::Value &arr) {
    std::vector<std::string> out;
    if (!arr.isArray()) return out;
    out.reserve(arr.size());
    for (const auto &v : arr) {
        if (v.isString())
            out.push_back(v.asString());
        else if (v.isInt())
            out.push_back(std::to_string(v.asInt()));
    }
    return out;
}

}  // namespace

GroupModule::GroupModule(IM::domain::service::IGroupService::Ptr group_service)
    : RockModule("svc.group", "0.1.0", "builtin"), m_group_service(std::move(group_service)) {}

bool GroupModule::onServerUp() {
    registerService("rock", "im", "svc-group");
    return true;
}

bool GroupModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                    IM::RockStream::ptr /*stream*/) {
    const auto cmd = request ? request->getCmd() : 0;

    if (!m_group_service) {
        WriteErr(response, 500, "group service not ready");
        return true;
    }

    Json::Value body;
    if (!IM::JsonUtil::FromString(body, request->getBody()) || !body.isObject()) {
        WriteErr(response, 400, "invalid json body");
        return true;
    }

    switch (cmd) {
        case kCmdCreateGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const std::string name = IM::JsonUtil::GetString(body, "name");
            auto members = ParseUint64ArrayAlias(body, "user_ids", "member_ids");
            auto r = m_group_service->CreateGroup(user_id, name, members);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value d(Json::objectValue);
            d["group_id"] = (Json::UInt64)r.data;
            WriteOk(response, d);
            return true;
        }
        case kCmdDismissGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_group_service->DismissGroup(user_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdGetGroupDetail: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_group_service->GetGroupDetail(user_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, GroupDetailToJson(r.data));
            return true;
        }
        case kCmdGetGroupList: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_group_service->GetGroupList(user_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(GroupItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdUpdateGroupSetting: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const std::string name = IM::JsonUtil::GetString(body, "name");
            const std::string avatar = IM::JsonUtil::GetString(body, "avatar");
            const std::string profile = IM::JsonUtil::GetString(body, "profile");
            auto r = m_group_service->UpdateGroupSetting(user_id, group_id, name, avatar, profile);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdHandoverGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const uint64_t new_owner_id = IM::JsonUtil::GetUint64(body, "new_owner_id");
            auto r = m_group_service->HandoverGroup(user_id, group_id, new_owner_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdAssignAdmin: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const uint64_t target_id = IM::JsonUtil::GetUint64(body, "target_id");
            const int action = IM::JsonUtil::GetInt32(body, "action");
            auto r = m_group_service->AssignAdmin(user_id, group_id, target_id, action);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdMuteGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const int action = IM::JsonUtil::GetInt32(body, "action");
            auto r = m_group_service->MuteGroup(user_id, group_id, action);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdOvertGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const int action = IM::JsonUtil::GetInt32(body, "action");
            auto r = m_group_service->OvertGroup(user_id, group_id, action);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdGetOvertGroupList: {
            const int page = IM::JsonUtil::GetInt32(body, "page");
            const std::string name = IM::JsonUtil::GetString(body, "name");
            auto r = m_group_service->GetOvertGroupList(page, name);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data.first) arr.append(GroupOvertItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            d["has_more"] = r.data.second;
            WriteOk(response, d);
            return true;
        }
        case kCmdGetGroupMemberList: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_group_service->GetGroupMemberList(user_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(GroupMemberItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdInviteGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto members = ParseUint64ArrayAlias(body, "user_ids", "member_ids");
            auto r = m_group_service->InviteGroup(user_id, group_id, members);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdRemoveMember: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto members = ParseUint64ArrayAlias(body, "user_ids", "member_ids");
            auto r = m_group_service->RemoveMember(user_id, group_id, members);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdSecedeGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_group_service->SecedeGroup(user_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdUpdateMemberRemark: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_group_service->UpdateMemberRemark(user_id, group_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdMuteMember: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const uint64_t target_id = IM::JsonUtil::GetUint64(body, "target_id");
            const int action = IM::JsonUtil::GetInt32(body, "action");
            auto r = m_group_service->MuteMember(user_id, group_id, target_id, action);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdCreateApply: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_group_service->CreateApply(user_id, group_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdAgreeApply: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t apply_id = IM::JsonUtil::GetUint64(body, "apply_id");
            auto r = m_group_service->AgreeApply(user_id, apply_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdDeclineApply: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t apply_id = IM::JsonUtil::GetUint64(body, "apply_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_group_service->DeclineApply(user_id, apply_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdGetApplyList: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_group_service->GetApplyList(user_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(GroupApplyItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdGetUserApplyList: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_group_service->GetUserApplyList(user_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(GroupApplyItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdGetUnreadApplyCount: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_group_service->GetUnreadApplyCount(user_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value d(Json::objectValue);
            d["num"] = r.data;
            WriteOk(response, d);
            return true;
        }
        case kCmdEditNotice: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const std::string content = IM::JsonUtil::GetString(body, "content");
            auto r = m_group_service->EditNotice(user_id, group_id, content);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdCreateVote: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            const std::string title = IM::JsonUtil::GetString(body, "title");
            const int answer_mode = IM::JsonUtil::GetInt32(body, "answer_mode");
            const int is_anonymous = IM::JsonUtil::GetInt32(body, "is_anonymous");
            auto options = ParseStringArray(body["options"]);
            auto r = m_group_service->CreateVote(user_id, group_id, title, answer_mode, is_anonymous, options);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value d(Json::objectValue);
            d["vote_id"] = (Json::UInt64)r.data;
            WriteOk(response, d);
            return true;
        }
        case kCmdGetVoteList: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_group_service->GetVoteList(user_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(GroupVoteItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdGetVoteDetail: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t vote_id = IM::JsonUtil::GetUint64(body, "vote_id");
            auto r = m_group_service->GetVoteDetail(user_id, vote_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, GroupVoteDetailToJson(r.data));
            return true;
        }
        case kCmdCastVote: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t vote_id = IM::JsonUtil::GetUint64(body, "vote_id");
            auto options = ParseStringArray(body["options"]);
            auto r = m_group_service->CastVote(user_id, vote_id, options);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdFinishVote: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t vote_id = IM::JsonUtil::GetUint64(body, "vote_id");
            auto r = m_group_service->FinishVote(user_id, vote_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        default:
            return false;
    }
}

bool GroupModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::group
