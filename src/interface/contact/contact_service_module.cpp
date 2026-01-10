#include "interface/contact/contact_service_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/util/json_util.hpp"

namespace IM::contact {

static auto g_logger = IM_LOG_NAME("root");

namespace {
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

void WriteOk(IM::RockResponse::ptr response, const Json::Value &data = Json::Value()) {
    Json::Value out(Json::objectValue);
    out["code"] = 200;
    if (!data.isNull()) {
        out["data"] = data;
    }
    response->setBody(IM::JsonUtil::ToString(out));
    response->setResult(200);
    response->setResultStr("ok");
}

void WriteErr(IM::RockResponse::ptr response, int code, const std::string &err) {
    response->setResult(code == 0 ? 500 : code);
    response->setResultStr(err.empty() ? "error" : err);
}

bool ParseJsonBody(const IM::RockRequest::ptr &request, Json::Value &out) {
    if (!request) return false;
    if (!IM::JsonUtil::FromString(out, request->getBody())) return false;
    return out.isObject();
}

Json::Value TalkSessionToJson(const IM::dto::TalkSessionItem &s) {
    Json::Value out(Json::objectValue);
    out["id"] = (Json::UInt64)s.id;
    out["talk_mode"] = (Json::UInt)s.talk_mode;
    out["to_from_id"] = (Json::UInt64)s.to_from_id;
    out["is_top"] = (Json::UInt)s.is_top;
    out["is_disturb"] = (Json::UInt)s.is_disturb;
    out["is_robot"] = (Json::UInt)s.is_robot;
    out["name"] = s.name;
    out["avatar"] = s.avatar;
    out["remark"] = s.remark;
    out["unread_num"] = (Json::UInt)s.unread_num;
    out["msg_text"] = s.msg_text;
    out["updated_at"] = s.updated_at;
    return out;
}

Json::Value UserToJson(const IM::model::User &u) {
    Json::Value out(Json::objectValue);
    out["user_id"] = (Json::UInt64)u.id;
    out["mobile"] = u.mobile;
    out["nickname"] = u.nickname;
    out["avatar"] = u.avatar;
    out["gender"] = (Json::UInt)u.gender;
    out["motto"] = u.motto;
    return out;
}

Json::Value ContactItemToJson(const IM::dto::ContactItem &c) {
    Json::Value out(Json::objectValue);
    out["user_id"] = (Json::UInt64)c.user_id;
    out["nickname"] = c.nickname;
    out["gender"] = (Json::UInt)c.gender;
    out["motto"] = c.motto;
    out["avatar"] = c.avatar;
    out["remark"] = c.remark;
    out["group_id"] = (Json::UInt64)c.group_id;
    return out;
}

Json::Value ContactApplyItemToJson(const IM::dto::ContactApplyItem &c) {
    Json::Value out(Json::objectValue);
    out["id"] = (Json::UInt64)c.id;
    out["apply_user_id"] = (Json::UInt64)c.apply_user_id;
    out["target_user_id"] = (Json::UInt64)c.target_user_id;
    out["remark"] = c.remark;
    out["nickname"] = c.nickname;
    out["avatar"] = c.avatar;
    out["created_at"] = c.created_at;
    return out;
}

Json::Value ContactGroupItemToJson(const IM::dto::ContactGroupItem &c) {
    Json::Value out(Json::objectValue);
    out["id"] = (Json::UInt64)c.id;
    out["name"] = c.name;
    out["count"] = (Json::UInt)c.contact_count;
    out["sort"] = (Json::UInt)c.sort;
    return out;
}

}  // namespace

ContactServiceModule::ContactServiceModule(IM::domain::service::IContactService::Ptr contact_service)
    : RockModule("svc.contact.biz", "0.1.0", "builtin"), m_contact_service(std::move(contact_service)) {}

bool ContactServiceModule::onServerUp() {
    // 由现有 ContactModule(查询模块)负责 registerService("svc-contact")
    return true;
}

bool ContactServiceModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                             IM::RockStream::ptr /*stream*/) {
    const auto cmd = request ? request->getCmd() : 0;
    if (cmd < kCmdAgreeApply || cmd > kCmdChangeContactGroup) {
        return false;
    }

    if (!m_contact_service) {
        WriteErr(response, 500, "contact service not ready");
        return true;
    }

    Json::Value body;
    if (!ParseJsonBody(request, body)) {
        WriteErr(response, 400, "invalid json body");
        return true;
    }

    switch (cmd) {
        case kCmdAgreeApply: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t apply_id = IM::JsonUtil::GetUint64(body, "apply_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_contact_service->AgreeApply(user_id, apply_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, TalkSessionToJson(r.data));
            return true;
        }
        case kCmdSearchByMobile: {
            const std::string mobile = IM::JsonUtil::GetString(body, "mobile");
            auto r = m_contact_service->SearchByMobile(mobile);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdListFriends: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_contact_service->ListFriends(user_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(ContactItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdCreateContactApply: {
            const uint64_t apply_user_id = IM::JsonUtil::GetUint64(body, "apply_user_id");
            const uint64_t target_user_id = IM::JsonUtil::GetUint64(body, "target_user_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_contact_service->CreateContactApply(apply_user_id, target_user_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdGetPendingContactApplyCount: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_contact_service->GetPendingContactApplyCount(user_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            Json::Value d(Json::objectValue);
            d["num"] = (Json::UInt64)r.data;
            WriteOk(response, d);
            return true;
        }
        case kCmdListContactApplies: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_contact_service->ListContactApplies(user_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(ContactApplyItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdRejectApply: {
            const uint64_t handler_user_id = IM::JsonUtil::GetUint64(body, "handler_user_id");
            const uint64_t apply_user_id = IM::JsonUtil::GetUint64(body, "apply_user_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_contact_service->RejectApply(handler_user_id, apply_user_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdEditContactRemark: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t contact_id = IM::JsonUtil::GetUint64(body, "contact_id");
            const std::string remark = IM::JsonUtil::GetString(body, "remark");
            auto r = m_contact_service->EditContactRemark(user_id, contact_id, remark);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdDeleteContact: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t contact_id = IM::JsonUtil::GetUint64(body, "contact_id");
            auto r = m_contact_service->DeleteContact(user_id, contact_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdSaveContactGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            std::vector<std::tuple<uint64_t, uint64_t, std::string>> groupItems;
            if (body.isMember("items") && body["items"].isArray()) {
                for (const auto &it : body["items"]) {
                    const uint64_t id = IM::JsonUtil::GetUint64(it, "id");
                    const uint64_t sort = IM::JsonUtil::GetUint64(it, "sort");
                    const std::string name = IM::JsonUtil::GetString(it, "name");
                    groupItems.emplace_back(id, sort, name);
                }
            }
            auto r = m_contact_service->SaveContactGroup(user_id, groupItems);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdGetContactGroupLists: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_contact_service->GetContactGroupLists(user_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            Json::Value arr(Json::arrayValue);
            for (const auto &it : r.data) arr.append(ContactGroupItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = arr;
            WriteOk(response, d);
            return true;
        }
        case kCmdChangeContactGroup: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t contact_id = IM::JsonUtil::GetUint64(body, "contact_id");
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            auto r = m_contact_service->ChangeContactGroup(user_id, contact_id, group_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        default:
            return false;
    }
}

bool ContactServiceModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::contact
