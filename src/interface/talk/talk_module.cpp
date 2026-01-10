#include "interface/talk/talk_module.hpp"

#include "core/base/macro.hpp"
#include "core/util/json_util.hpp"

namespace IM::talk {

static auto g_logger = IM_LOG_NAME("root");

namespace {

static void WriteOk(const IM::RockResponse::ptr &response, const Json::Value &data) {
    if (!response) return;
    Json::Value out(Json::objectValue);
    out["code"] = 200;
    out["data"] = data;
    response->setResult(200);
    response->setBody(IM::JsonUtil::ToString(out));
}

static void WriteErr(const IM::RockResponse::ptr &response, int code, const std::string &msg) {
    if (!response) return;
    Json::Value out(Json::objectValue);
    out["code"] = code;
    out["message"] = msg;
    out["data"] = Json::Value();
    response->setResult(code);
    response->setResultStr(msg);
    response->setBody(IM::JsonUtil::ToString(out));
}

static Json::Value TalkSessionItemToJson(const IM::dto::TalkSessionItem &it) {
    Json::Value j(Json::objectValue);
    j["id"] = (Json::UInt64)it.id;
    j["talk_mode"] = it.talk_mode;
    j["to_from_id"] = (Json::UInt64)it.to_from_id;
    j["is_top"] = it.is_top;
    j["is_disturb"] = it.is_disturb;
    j["is_robot"] = it.is_robot;
    j["name"] = it.name;
    j["avatar"] = it.avatar;
    j["remark"] = it.remark;
    j["unread_num"] = it.unread_num;
    j["msg_text"] = it.msg_text;
    j["updated_at"] = it.updated_at;
    return j;
}

// cmd allocation (talk domain)
constexpr uint32_t kCmdGetSessionList = 701;
constexpr uint32_t kCmdSetSessionTop = 702;
constexpr uint32_t kCmdSetSessionDisturb = 703;
constexpr uint32_t kCmdCreateSession = 704;
constexpr uint32_t kCmdDeleteSession = 705;
constexpr uint32_t kCmdClearUnread = 706;

// ws query cmd allocation (talk domain)
constexpr uint32_t kCmdGetGroupTalkId = 707;
constexpr uint32_t kCmdListUsersByTalkId = 708;

}  // namespace

TalkModule::TalkModule(IM::domain::service::ITalkService::Ptr talk_service,
                       IM::domain::repository::ITalkRepository::Ptr talk_repo)
    : RockModule("svc.talk", "0.1.0", "builtin"),
      m_talk_service(std::move(talk_service)),
      m_talk_repo(std::move(talk_repo)) {}

bool TalkModule::onServerUp() {
    registerService("rock", "im", "svc-talk");
    return true;
}

bool TalkModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                   IM::RockStream::ptr /*stream*/) {
    const auto cmd = request ? request->getCmd() : 0;

    if (!m_talk_service) {
        WriteErr(response, 500, "talk service not ready");
        return true;
    }

    Json::Value body;
    if (!IM::JsonUtil::FromString(body, request->getBody()) || !body.isObject()) {
        WriteErr(response, 400, "invalid json body");
        return true;
    }

    switch (cmd) {
        case kCmdGetSessionList: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            auto r = m_talk_service->getSessionListByUserId(user_id);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            Json::Value items(Json::arrayValue);
            for (const auto &it : r.data) items.append(TalkSessionItemToJson(it));
            Json::Value d(Json::objectValue);
            d["items"] = items;
            WriteOk(response, d);
            return true;
        }
        case kCmdSetSessionTop: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");
            const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
            const uint8_t action = IM::JsonUtil::GetUint8(body, "action");
            auto r = m_talk_service->setSessionTop(user_id, to_from_id, talk_mode, action);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdSetSessionDisturb: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");
            const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
            const uint8_t action = IM::JsonUtil::GetUint8(body, "action");
            auto r = m_talk_service->setSessionDisturb(user_id, to_from_id, talk_mode, action);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdCreateSession: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");
            const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
            auto r = m_talk_service->createSession(user_id, to_from_id, talk_mode);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, TalkSessionItemToJson(r.data));
            return true;
        }
        case kCmdDeleteSession: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");
            const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
            auto r = m_talk_service->deleteSession(user_id, to_from_id, talk_mode);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdClearUnread: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");
            const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
            auto r = m_talk_service->clearSessionUnreadNum(user_id, to_from_id, talk_mode);
            if (!r.ok) {
                WriteErr(response, r.code == 0 ? 500 : r.code, r.err);
                return true;
            }
            WriteOk(response, Json::Value(Json::objectValue));
            return true;
        }
        case kCmdGetGroupTalkId: {
            if (!m_talk_repo) {
                WriteErr(response, 500, "talk repo not ready");
                return true;
            }
            const uint64_t group_id = IM::JsonUtil::GetUint64(body, "group_id");
            IM_LOG_INFO(g_logger) << "svc-talk cmd707 getGroupTalkId group_id=" << group_id;
            uint64_t talk_id = 0;
            std::string err;
            if (!m_talk_repo->getGroupTalkId(group_id, talk_id, &err)) {
                WriteErr(response, 404, err.empty() ? "talk not found" : err);
                return true;
            }
            IM_LOG_INFO(g_logger) << "svc-talk cmd707 getGroupTalkId ok talk_id=" << talk_id;
            Json::Value d(Json::objectValue);
            d["talk_id"] = (Json::UInt64)talk_id;
            WriteOk(response, d);
            return true;
        }
        case kCmdListUsersByTalkId: {
            if (!m_talk_repo) {
                WriteErr(response, 500, "talk repo not ready");
                return true;
            }
            const uint64_t talk_id = IM::JsonUtil::GetUint64(body, "talk_id");
            IM_LOG_INFO(g_logger) << "svc-talk cmd708 listUsersByTalkId talk_id=" << talk_id;
            std::vector<uint64_t> users;
            std::string err;
            if (!m_talk_repo->listUsersByTalkId(talk_id, users, &err)) {
                WriteErr(response, 500, err.empty() ? "list users failed" : err);
                return true;
            }
            IM_LOG_INFO(g_logger) << "svc-talk cmd708 listUsersByTalkId ok users=" << users.size();
            Json::Value arr(Json::arrayValue);
            for (auto uid : users) arr.append((Json::UInt64)uid);
            Json::Value d(Json::objectValue);
            d["user_ids"] = arr;
            WriteOk(response, d);
            return true;
        }
        default:
            WriteErr(response, 404, "unknown cmd");
            return true;
    }
}

bool TalkModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::talk
