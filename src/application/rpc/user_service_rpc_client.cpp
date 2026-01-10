#include "application/rpc/user_service_rpc_client.hpp"

#include "core/system/application.hpp"
#include "core/util/json_util.hpp"

namespace IM::app::rpc {

namespace {
constexpr uint32_t kTimeoutMs = 3000;

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

UserServiceRpcClient::UserServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("user.rpc_addr", std::string(""), "svc-user rpc address ip:port")) {}

IM::RockResult::ptr UserServiceRpcClient::rockJsonRequest(const std::string &ip_port, uint32_t cmd,
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

std::string UserServiceRpcClient::resolveSvcUserAddr() {
    const auto fixed = m_rpc_addr->getValue();
    if (!fixed.empty()) return fixed;

    if (auto sd = IM::Application::GetInstance()->getServiceDiscovery()) {
        std::unordered_map<std::string,
                           std::unordered_map<std::string, std::unordered_map<uint64_t, IM::ServiceItemInfo::ptr>>>
            infos;
        sd->listServer(infos);
        auto itD = infos.find("im");
        if (itD == infos.end()) {
            sd->queryServer("im", "svc-user");
            return "";
        }
        auto itS = itD->second.find("svc-user");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-user");
            return "";
        }
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }
    return "";
}

bool UserServiceRpcClient::parseUser(const Json::Value &j, IM::model::User &out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetUint64(j, "id");
    out.mobile = IM::JsonUtil::GetString(j, "mobile");
    out.email = IM::JsonUtil::GetString(j, "email");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.avatar_media_id = IM::JsonUtil::GetString(j, "avatar_media_id");
    out.motto = IM::JsonUtil::GetString(j, "motto");
    out.birthday = IM::JsonUtil::GetString(j, "birthday");
    out.gender = IM::JsonUtil::GetUint8(j, "gender");
    out.online_status = IM::JsonUtil::GetString(j, "online_status");
    out.is_qiye = IM::JsonUtil::GetUint8(j, "is_qiye");
    out.is_robot = IM::JsonUtil::GetUint8(j, "is_robot");
    out.is_disabled = IM::JsonUtil::GetUint8(j, "is_disabled");
    return out.id != 0;
}

bool UserServiceRpcClient::parseUserInfo(const Json::Value &j, IM::dto::UserInfo &out) {
    if (!j.isObject()) return false;
    out.uid = IM::JsonUtil::GetUint64(j, "uid");
    out.nickname = IM::JsonUtil::GetString(j, "nickname");
    out.avatar = IM::JsonUtil::GetString(j, "avatar");
    out.motto = IM::JsonUtil::GetString(j, "motto");
    out.gender = IM::JsonUtil::GetUint8(j, "gender");
    out.is_qiye = IM::JsonUtil::GetUint8(j, "is_qiye");
    out.mobile = IM::JsonUtil::GetString(j, "mobile");
    out.email = IM::JsonUtil::GetString(j, "email");
    return out.uid != 0;
}

bool UserServiceRpcClient::parseUserSettings(const Json::Value &j, IM::model::UserSettings &out) {
    if (!j.isObject()) return false;
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.theme_mode = IM::JsonUtil::GetString(j, "theme_mode");
    out.theme_bag_img = IM::JsonUtil::GetString(j, "theme_bag_img");
    out.theme_color = IM::JsonUtil::GetString(j, "theme_color");
    out.notify_cue_tone = IM::JsonUtil::GetString(j, "notify_cue_tone");
    out.keyboard_event_notify = IM::JsonUtil::GetString(j, "keyboard_event_notify");
    return out.user_id != 0;
}

Result<IM::model::User> UserServiceRpcClient::LoadUserInfo(const uint64_t uid) {
    Result<IM::model::User> result;
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)uid;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdLoadUserInfo, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
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

Result<void> UserServiceRpcClient::UpdatePassword(const uint64_t uid, const std::string &old_password,
                                                  const std::string &new_password) {
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)uid;
    req["old_password"] = old_password;
    req["new_password"] = new_password;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdUpdatePassword, req, kTimeoutMs), "svc-user unavailable");
}

Result<void> UserServiceRpcClient::UpdateUserInfo(const uint64_t uid, const std::string &nickname,
                                                  const std::string &avatar, const std::string &motto,
                                                  const uint32_t gender, const std::string &birthday) {
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)uid;
    req["nickname"] = nickname;
    req["avatar"] = avatar;
    req["motto"] = motto;
    req["gender"] = (Json::UInt)gender;
    req["birthday"] = birthday;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdUpdateUserInfo, req, kTimeoutMs), "svc-user unavailable");
}

Result<void> UserServiceRpcClient::UpdateMobile(const uint64_t uid, const std::string &password,
                                                const std::string &new_mobile, const std::string &sms_code) {
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)uid;
    req["password"] = password;
    req["new_mobile"] = new_mobile;
    req["sms_code"] = sms_code;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdUpdateMobile, req, kTimeoutMs), "svc-user unavailable");
}

Result<void> UserServiceRpcClient::UpdateEmail(const uint64_t uid, const std::string &password,
                                               const std::string &new_email, const std::string &email_code) {
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)uid;
    req["password"] = password;
    req["new_email"] = new_email;
    req["email_code"] = email_code;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdUpdateEmail, req, kTimeoutMs), "svc-user unavailable");
}

Result<IM::model::User> UserServiceRpcClient::GetUserByMobile(const std::string &mobile, const std::string &channel) {
    Result<IM::model::User> result;
    Json::Value req(Json::objectValue);
    req["mobile"] = mobile;
    req["channel"] = channel;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdGetUserByMobile, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
        return result;
    }

    // 约定：
    // - channel=register：手机号未注册是正常情况，svc-user 可能返回空 data 或 uid=0。
    // - channel=forget_account：手机号必须已注册，svc-user 应返回有效 user。
    if (!out.isMember("data") || !out["data"].isObject()) {
        if (channel == "register") {
            result.ok = true;
            return result;
        }
        result.code = 500;
        result.err = "invalid svc-user response";
        return result;
    }

    IM::model::User u;
    if (!parseUser(out["data"], u)) {
        if (channel == "register") {
            result.ok = true;
            return result;
        }
        result.code = 500;
        result.err = "invalid user";
        return result;
    }

    result.data = std::move(u);
    result.ok = true;
    return result;
}

Result<IM::model::User> UserServiceRpcClient::GetUserByEmail(const std::string &email, const std::string &channel) {
    Result<IM::model::User> result;
    Json::Value req(Json::objectValue);
    req["email"] = email;
    req["channel"] = channel;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdGetUserByEmail, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
        return result;
    }

    // channel=register / update_email：邮箱未注册是正常情况，允许空 data 或 uid=0。
    if (!out.isMember("data") || !out["data"].isObject()) {
        if (channel == "register" || channel == "update_email") {
            result.ok = true;
            return result;
        }
        result.code = 500;
        result.err = "invalid svc-user response";
        return result;
    }

    IM::model::User u;
    if (!parseUser(out["data"], u)) {
        if (channel == "register" || channel == "update_email") {
            result.ok = true;
            return result;
        }
        result.code = 500;
        result.err = "invalid user";
        return result;
    }

    result.data = std::move(u);
    result.ok = true;
    return result;
}

Result<void> UserServiceRpcClient::Offline(const uint64_t id) {
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)id;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdOffline, req, kTimeoutMs), "svc-user unavailable");
}

Result<std::string> UserServiceRpcClient::GetUserOnlineStatus(const uint64_t id) {
    Result<std::string> result;
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)id;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdGetUserOnlineStatus, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
        return result;
    }

    const auto online_status = IM::JsonUtil::GetString(out["data"], "online_status");
    result.data = online_status;
    result.ok = true;
    return result;
}

Result<void> UserServiceRpcClient::SaveConfigInfo(const uint64_t user_id, const std::string &theme_mode,
                                                  const std::string &theme_bag_img, const std::string &theme_color,
                                                  const std::string &notify_cue_tone,
                                                  const std::string &keyboard_event_notify) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["theme_mode"] = theme_mode;
    req["theme_bag_img"] = theme_bag_img;
    req["theme_color"] = theme_color;
    req["notify_cue_tone"] = notify_cue_tone;
    req["keyboard_event_notify"] = keyboard_event_notify;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdSaveConfigInfo, req, kTimeoutMs), "svc-user unavailable");
}

Result<IM::model::UserSettings> UserServiceRpcClient::LoadConfigInfo(const uint64_t user_id) {
    Result<IM::model::UserSettings> result;
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdLoadConfigInfo, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
        return result;
    }

    IM::model::UserSettings s;
    if (!out.isMember("data") || !parseUserSettings(out["data"], s)) {
        result.code = 500;
        result.err = "invalid user settings";
        return result;
    }

    result.data = std::move(s);
    result.ok = true;
    return result;
}

Result<IM::dto::UserInfo> UserServiceRpcClient::LoadUserInfoSimple(const uint64_t uid) {
    Result<IM::dto::UserInfo> result;
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)uid;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdLoadUserInfoSimple, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
        return result;
    }

    IM::dto::UserInfo u;
    if (!out.isMember("data") || !parseUserInfo(out["data"], u)) {
        result.code = 500;
        result.err = "invalid user info";
        return result;
    }

    result.data = std::move(u);
    result.ok = true;
    return result;
}

Result<IM::model::User> UserServiceRpcClient::Authenticate(const std::string &mobile, const std::string &password,
                                                           const std::string &platform) {
    Result<IM::model::User> result;
    Json::Value req(Json::objectValue);
    req["mobile"] = mobile;
    req["password"] = password;
    req["platform"] = platform;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdAuthenticate, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
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

Result<void> UserServiceRpcClient::LogLogin(const Result<IM::model::User> &user_result, const std::string &platform,
                                            IM::http::HttpSession::ptr session) {
    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_result.data.id;
    req["mobile"] = user_result.data.mobile;
    req["platform"] = platform;
    req["success"] = (Json::UInt)(user_result.ok ? 1 : 0);
    req["reason"] = user_result.ok ? "" : user_result.err;

    std::string ip;
    if (session) {
        ip = session->getRemoteAddressString();
    }
    req["ip"] = ip;
    req["user_agent"] = "";

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdLogLogin, req, kTimeoutMs), "svc-user unavailable");
}

Result<void> UserServiceRpcClient::GoOnline(const uint64_t id) {
    Json::Value req(Json::objectValue);
    req["uid"] = (Json::UInt64)id;

    const auto addr = resolveSvcUserAddr();
    return FromRockVoid(rockJsonRequest(addr, kCmdGoOnline, req, kTimeoutMs), "svc-user unavailable");
}

Result<IM::model::User> UserServiceRpcClient::Register(const std::string &nickname, const std::string &mobile,
                                                       const std::string &password, const std::string &sms_code,
                                                       const std::string &platform) {
    Result<IM::model::User> result;
    Json::Value req(Json::objectValue);
    req["nickname"] = nickname;
    req["mobile"] = mobile;
    req["password"] = password;
    req["sms_code"] = sms_code;
    req["platform"] = platform;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdRegister, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
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

Result<IM::model::User> UserServiceRpcClient::Forget(const std::string &mobile, const std::string &new_password,
                                                     const std::string &sms_code) {
    Result<IM::model::User> result;
    Json::Value req(Json::objectValue);
    req["mobile"] = mobile;
    req["new_password"] = new_password;
    req["sms_code"] = sms_code;

    const auto addr = resolveSvcUserAddr();
    auto rr = rockJsonRequest(addr, kCmdForget, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-user unavailable";
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
        result.err = "invalid svc-user response";
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

}  // namespace IM::app::rpc
