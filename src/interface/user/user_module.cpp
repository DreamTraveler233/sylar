#include "interface/user/user_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/util/json_util.hpp"
#include "core/util/time_util.hpp"

namespace IM::user {

static auto g_logger = IM_LOG_NAME("root");

namespace {
// User service RPC command range: 501+
constexpr uint32_t kCmdLoadUserInfo = 501;
constexpr uint32_t kCmdUpdatePassword = 502;
constexpr uint32_t kCmdUpdateUserInfo = 503;
constexpr uint32_t kCmdUpdateMobile = 504;
constexpr uint32_t kCmdUpdateEmail = 505;
constexpr uint32_t kCmdGetUserByMobile = 506;
constexpr uint32_t kCmdGetUserByEmail = 507;
constexpr uint32_t kCmdOffline = 508;
constexpr uint32_t kCmdGetUserOnlineStatus = 509;
constexpr uint32_t kCmdSaveConfigInfo = 510;
constexpr uint32_t kCmdLoadConfigInfo = 511;
constexpr uint32_t kCmdLoadUserInfoSimple = 512;
constexpr uint32_t kCmdAuthenticate = 513;
constexpr uint32_t kCmdLogLogin = 514;
constexpr uint32_t kCmdGoOnline = 515;
constexpr uint32_t kCmdRegister = 516;
constexpr uint32_t kCmdForget = 517;

Json::Value UserToJson(const IM::model::User& u) {
    Json::Value out(Json::objectValue);
    out["id"] = (Json::UInt64)u.id;
    out["mobile"] = u.mobile;
    out["email"] = u.email;
    out["nickname"] = u.nickname;
    out["avatar"] = u.avatar;
    out["avatar_media_id"] = u.avatar_media_id;
    out["motto"] = u.motto;
    out["birthday"] = u.birthday;
    out["gender"] = (Json::UInt)u.gender;
    out["online_status"] = u.online_status;
    out["is_qiye"] = (Json::UInt)u.is_qiye;
    out["is_robot"] = (Json::UInt)u.is_robot;
    out["is_disabled"] = (Json::UInt)u.is_disabled;
    return out;
}

Json::Value UserInfoToJson(const IM::dto::UserInfo& u) {
    Json::Value out(Json::objectValue);
    out["uid"] = (Json::UInt64)u.uid;
    out["nickname"] = u.nickname;
    out["avatar"] = u.avatar;
    out["motto"] = u.motto;
    out["gender"] = (Json::UInt)u.gender;
    out["is_qiye"] = (Json::UInt)u.is_qiye;
    out["mobile"] = u.mobile;
    out["email"] = u.email;
    return out;
}

Json::Value UserSettingsToJson(const IM::model::UserSettings& s) {
    Json::Value out(Json::objectValue);
    out["user_id"] = (Json::UInt64)s.user_id;
    out["theme_mode"] = s.theme_mode;
    out["theme_bag_img"] = s.theme_bag_img;
    out["theme_color"] = s.theme_color;
    out["notify_cue_tone"] = s.notify_cue_tone;
    out["keyboard_event_notify"] = s.keyboard_event_notify;
    return out;
}

void WriteOk(IM::RockResponse::ptr response, const Json::Value& data = Json::Value()) {
    Json::Value out(Json::objectValue);
    out["code"] = 200;
    if (!data.isNull()) {
        out["data"] = data;
    }
    response->setBody(IM::JsonUtil::ToString(out));
    response->setResult(200);
    response->setResultStr("ok");
}

void WriteErr(IM::RockResponse::ptr response, int code, const std::string& err) {
    response->setResult(code == 0 ? 500 : code);
    response->setResultStr(err.empty() ? "error" : err);
}

bool ParseJsonBody(const IM::RockRequest::ptr& request, Json::Value& out) {
    if (!request) return false;
    if (!IM::JsonUtil::FromString(out, request->getBody())) return false;
    return out.isObject();
}

}  // namespace

UserModule::UserModule(IM::domain::service::IUserService::Ptr user_service,
                       IM::domain::repository::IUserRepository::Ptr user_repo)
    : RockModule("svc.user", "0.1.0", "builtin"),
      m_user_service(std::move(user_service)),
      m_user_repo(std::move(user_repo)) {}

bool UserModule::onServerUp() {
    registerService("rock", "im", "svc-user");
    return true;
}

bool UserModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                  IM::RockStream::ptr /*stream*/) {
    const auto cmd = request ? request->getCmd() : 0;

    if (cmd < kCmdLoadUserInfo || cmd > kCmdForget) {
        return false;
    }

    Json::Value body;
    if (!ParseJsonBody(request, body)) {
        response->setResult(400);
        response->setResultStr("invalid json body");
        return true;
    }

    if (!m_user_service && cmd != kCmdLogLogin) {
        response->setResult(500);
        response->setResultStr("user service not ready");
        return true;
    }

    switch (cmd) {
        case kCmdLoadUserInfo: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            if (uid == 0) {
                response->setResult(400);
                response->setResultStr("missing uid");
                return true;
            }
            auto r = m_user_service->LoadUserInfo(uid);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdUpdatePassword: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            const std::string old_password = IM::JsonUtil::GetString(body, "old_password");
            const std::string new_password = IM::JsonUtil::GetString(body, "new_password");
            if (uid == 0 || old_password.empty() || new_password.empty()) {
                response->setResult(400);
                response->setResultStr("missing uid/old_password/new_password");
                return true;
            }
            auto r = m_user_service->UpdatePassword(uid, old_password, new_password);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdUpdateUserInfo: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            const std::string nickname = IM::JsonUtil::GetString(body, "nickname");
            const std::string avatar = IM::JsonUtil::GetString(body, "avatar");
            const std::string motto = IM::JsonUtil::GetString(body, "motto");
            const uint32_t gender = IM::JsonUtil::GetUint32(body, "gender");
            const std::string birthday = IM::JsonUtil::GetString(body, "birthday");
            if (uid == 0) {
                response->setResult(400);
                response->setResultStr("missing uid");
                return true;
            }
            auto r = m_user_service->UpdateUserInfo(uid, nickname, avatar, motto, gender, birthday);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdUpdateMobile: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            const std::string password = IM::JsonUtil::GetString(body, "password");
            const std::string new_mobile = IM::JsonUtil::GetString(body, "new_mobile");
            const std::string sms_code = IM::JsonUtil::GetString(body, "sms_code");
            if (uid == 0 || password.empty() || new_mobile.empty() || sms_code.empty()) {
                response->setResult(400);
                response->setResultStr("missing fields");
                return true;
            }
            auto r = m_user_service->UpdateMobile(uid, password, new_mobile, sms_code);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdUpdateEmail: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            const std::string password = IM::JsonUtil::GetString(body, "password");
            const std::string new_email = IM::JsonUtil::GetString(body, "new_email");
            const std::string email_code = IM::JsonUtil::GetString(body, "email_code");
            if (uid == 0 || password.empty() || new_email.empty() || email_code.empty()) {
                response->setResult(400);
                response->setResultStr("missing fields");
                return true;
            }
            auto r = m_user_service->UpdateEmail(uid, password, new_email, email_code);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdGetUserByMobile: {
            const std::string mobile = IM::JsonUtil::GetString(body, "mobile");
            const std::string channel = IM::JsonUtil::GetString(body, "channel");
            if (mobile.empty()) {
                response->setResult(400);
                response->setResultStr("missing mobile");
                return true;
            }
            auto r = m_user_service->GetUserByMobile(mobile, channel);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdGetUserByEmail: {
            const std::string email = IM::JsonUtil::GetString(body, "email");
            const std::string channel = IM::JsonUtil::GetString(body, "channel");
            if (email.empty()) {
                response->setResult(400);
                response->setResultStr("missing email");
                return true;
            }
            auto r = m_user_service->GetUserByEmail(email, channel);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdOffline: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            if (uid == 0) {
                response->setResult(400);
                response->setResultStr("missing uid");
                return true;
            }
            auto r = m_user_service->Offline(uid);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdGetUserOnlineStatus: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            if (uid == 0) {
                response->setResult(400);
                response->setResultStr("missing uid");
                return true;
            }
            auto r = m_user_service->GetUserOnlineStatus(uid);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            Json::Value data(Json::objectValue);
            data["online_status"] = r.data;
            WriteOk(response, data);
            return true;
        }
        case kCmdSaveConfigInfo: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            const std::string theme_mode = IM::JsonUtil::GetString(body, "theme_mode");
            const std::string theme_bag_img = IM::JsonUtil::GetString(body, "theme_bag_img");
            const std::string theme_color = IM::JsonUtil::GetString(body, "theme_color");
            const std::string notify_cue_tone = IM::JsonUtil::GetString(body, "notify_cue_tone");
            const std::string keyboard_event_notify =
                IM::JsonUtil::GetString(body, "keyboard_event_notify");
            if (user_id == 0) {
                response->setResult(400);
                response->setResultStr("missing user_id");
                return true;
            }
            auto r = m_user_service->SaveConfigInfo(user_id, theme_mode, theme_bag_img, theme_color,
                                                   notify_cue_tone, keyboard_event_notify);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdLoadConfigInfo: {
            const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
            if (user_id == 0) {
                response->setResult(400);
                response->setResultStr("missing user_id");
                return true;
            }
            auto r = m_user_service->LoadConfigInfo(user_id);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserSettingsToJson(r.data));
            return true;
        }
        case kCmdLoadUserInfoSimple: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            if (uid == 0) {
                response->setResult(400);
                response->setResultStr("missing uid");
                return true;
            }
            auto r = m_user_service->LoadUserInfoSimple(uid);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserInfoToJson(r.data));
            return true;
        }
        case kCmdAuthenticate: {
            const std::string mobile = IM::JsonUtil::GetString(body, "mobile");
            const std::string password = IM::JsonUtil::GetString(body, "password");
            const std::string platform = IM::JsonUtil::GetString(body, "platform");
            if (mobile.empty() || password.empty()) {
                response->setResult(400);
                response->setResultStr("missing mobile/password");
                return true;
            }
            auto r = m_user_service->Authenticate(mobile, password, platform);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdGoOnline: {
            const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
            if (uid == 0) {
                response->setResult(400);
                response->setResultStr("missing uid");
                return true;
            }
            auto r = m_user_service->GoOnline(uid);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response);
            return true;
        }
        case kCmdRegister: {
            const std::string nickname = IM::JsonUtil::GetString(body, "nickname");
            const std::string mobile = IM::JsonUtil::GetString(body, "mobile");
            const std::string password = IM::JsonUtil::GetString(body, "password");
            const std::string sms_code = IM::JsonUtil::GetString(body, "sms_code");
            const std::string platform = IM::JsonUtil::GetString(body, "platform");
            if (nickname.empty() || mobile.empty() || password.empty() || sms_code.empty()) {
                response->setResult(400);
                response->setResultStr("missing fields");
                return true;
            }
            auto r = m_user_service->Register(nickname, mobile, password, sms_code, platform);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdForget: {
            const std::string mobile = IM::JsonUtil::GetString(body, "mobile");
            const std::string new_password = IM::JsonUtil::GetString(body, "new_password");
            const std::string sms_code = IM::JsonUtil::GetString(body, "sms_code");
            if (mobile.empty() || new_password.empty() || sms_code.empty()) {
                response->setResult(400);
                response->setResultStr("missing fields");
                return true;
            }
            auto r = m_user_service->Forget(mobile, new_password, sms_code);
            if (!r.ok) {
                WriteErr(response, r.code, r.err);
                return true;
            }
            WriteOk(response, UserToJson(r.data));
            return true;
        }
        case kCmdLogLogin: {
            if (!m_user_repo) {
                response->setResult(500);
                response->setResultStr("user repo not ready");
                return true;
            }

            IM::model::UserLoginLog log;
            log.user_id = IM::JsonUtil::GetUint64(body, "user_id");
            log.mobile = IM::JsonUtil::GetString(body, "mobile");
            log.platform = IM::JsonUtil::GetString(body, "platform");
            log.ip = IM::JsonUtil::GetString(body, "ip");
            log.user_agent = IM::JsonUtil::GetString(body, "user_agent");
            log.success = (uint8_t)IM::JsonUtil::GetUint32(body, "success");
            log.reason = IM::JsonUtil::GetString(body, "reason");
            log.created_at = IM::TimeUtil::NowToS();

            if (log.user_id == 0) {
                response->setResult(400);
                response->setResultStr("missing user_id");
                return true;
            }

            std::string err;
            if (!m_user_repo->CreateUserLoginLog(log, &err)) {
                IM_LOG_ERROR(g_logger) << "CreateUserLoginLog failed, uid=" << log.user_id
                                       << ", err=" << err;
                response->setResult(500);
                response->setResultStr("记录登录日志失败");
                return true;
            }

            WriteOk(response);
            return true;
        }
        default:
            break;
    }

    return false;
}

bool UserModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::user
