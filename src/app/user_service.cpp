#include "app/user_service.hpp"

#include "common/common.hpp"
#include "crypto_module.hpp"
#include "dao/user_dao.hpp"
#include "macro.hpp"
#include "util/hash_util.hpp"
#include "util/password.hpp"

namespace CIM::app {

static auto g_logger = CIM_LOG_NAME("root");
UserResult UserService::LoadUserInfo(const uint64_t uid) {
    UserResult result;
    std::string err;
    if (!CIM::dao::UserDAO::GetById(uid, result.data, &err)) {
        CIM_LOG_ERROR(g_logger) << "LoadUserInfo failed, uid=" << uid << ", err=" << err;
        result.code = 404;
        result.err = "用户不存在！";
        return result;
    }
    result.ok = true;
    return result;
}

ResultVoid UserService::UpdatePassword(const uint64_t uid, const std::string& old_password,
                                       const std::string& new_password) {
    ResultVoid result;
    std::string err;

    // 1、密码解密
    std::string decrypted_old, decrypted_new;
    auto dec_old_res = CIM::DecryptPassword(old_password, decrypted_old);
    if (!dec_old_res.ok) {
        result.code = dec_old_res.code;
        result.err = dec_old_res.err;
        return result;
    }
    auto dec_new_res = CIM::DecryptPassword(new_password, decrypted_new);
    if (!dec_new_res.ok) {
        result.code = dec_new_res.code;
        result.err = dec_new_res.err;
        return result;
    }

    // 2、验证旧密码
    CIM::dao::User user;
    if (!CIM::dao::UserDAO::GetById(uid, user, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdatePassword GetById failed, uid=" << uid << ", err=" << err;
        result.code = 404;
        result.err = "用户不存在！";
        return result;
    }
    if (!CIM::util::Password::Verify(decrypted_old, user.password_hash)) {
        result.code = 403;
        result.err = "旧密码错误！";
        return result;
    }

    // 3、生成新密码哈希
    auto new_password_hash = CIM::util::Password::Hash(decrypted_new);
    if (new_password_hash.empty()) {
        CIM_LOG_ERROR(g_logger) << "UpdatePassword Hash failed, uid=" << uid;
        result.code = 500;
        result.err = "新密码哈希生成失败！";
        return result;
    }

    // 4、更新新密码
    if (!CIM::dao::UserDAO::UpdatePassword(uid, new_password_hash, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdatePassword failed, uid=" << uid << ", err=" << err;
        result.code = 500;
        result.err = "更新密码失败！";
        return result;
    }
    result.ok = true;
    return result;
}
ResultVoid UserService::UpdateUserInfo(const uint64_t uid, const std::string& nickname,
                                       const std::string& avatar, const std::string& motto,
                                       const uint32_t gender, const std::string& birthday) {
    ResultVoid result;
    std::string err;
    if (!CIM::dao::UserDAO::UpdateUserInfo(uid, nickname, avatar, motto, gender, birthday, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdateUserInfo failed, uid=" << uid << ", err=" << err;
        result.code = 500;
        result.err = "更新用户信息失败！";
        return result;
    }
    result.ok = true;
    return result;
}

ResultVoid UserService::UpdateMobile(const uint64_t uid, const std::string& password,
                                     const std::string& new_mobile) {
    ResultVoid result;
    std::string err;

    // 解密前端传入的登录密码
    std::string decrypted_password;
    auto dec_res = CIM::DecryptPassword(password, decrypted_password);
    if (!dec_res.ok) {
        result.code = dec_res.code;
        result.err = dec_res.err;
        return result;
    }

    // 加载当前用户信息用于校验
    CIM::dao::User user;
    if (!CIM::dao::UserDAO::GetById(uid, user, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdateMobile GetById failed, uid=" << uid << ", err=" << err;
        result.code = 404;
        result.err = "用户不存在！";
        return result;
    }

    if (!CIM::util::Password::Verify(decrypted_password, user.password_hash)) {
        result.code = 403;
        result.err = "密码错误！";
        return result;
    }

    if (user.mobile == new_mobile) {
        result.code = 400;
        result.err = "新手机号不能与原手机号相同！";
        return result;
    }

    // 检查新手机号是否已被其他账户使用
    CIM::dao::User other_user;
    std::string lookup_err;
    if (CIM::dao::UserDAO::GetByMobile(new_mobile, other_user, &lookup_err)) {
        if (other_user.id != uid) {
            result.code = 400;
            result.err = "新手机号已被使用！";
            return result;
        }
    } else if (!lookup_err.empty() && lookup_err != "user not found") {
        CIM_LOG_ERROR(g_logger) << "UpdateMobile GetByMobile failed, mobile=" << new_mobile
                                << ", err=" << lookup_err;
        result.code = 500;
        result.err = "校验新手机号失败！";
        return result;
    }

    if (!CIM::dao::UserDAO::UpdateMobile(uid, new_mobile, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdateMobile UpdateMobile failed, uid=" << uid
                                << ", err=" << err;
        result.code = 500;
        result.err = "更新手机号失败！";
        return result;
    }

    result.ok = true;
    return result;
}

UserResult UserService::GetUserByMobile(const std::string& mobile, const std::string& channel) {
    UserResult result;
    std::string err;

    if (channel == "register") {
        if (CIM::dao::UserDAO::GetByMobile(mobile, result.data, &err)) {
            CIM_LOG_ERROR(g_logger)
                << "GetUserByMobile failed, mobile=" << mobile << ", err=" << err;
            result.code = 400;
            result.err = "手机号已注册!";
            return result;
        }
    } else if (channel == "forget_account") {
        if (!CIM::dao::UserDAO::GetByMobile(mobile, result.data, &err)) {
            CIM_LOG_ERROR(g_logger)
                << "GetUserByMobile failed, mobile=" << mobile << ", err=" << err;
            result.code = 400;
            result.err = "手机号未注册!";
            return result;
        }
    }
    result.ok = true;
    return result;
}

UserResult UserService::Register(const std::string& nickname, const std::string& mobile,
                                 const std::string& password, const std::string& platform) {
    UserResult result;
    std::string err;

    /*密码解密*/
    std::string decrypted_pwd;
    auto dec_res = CIM::DecryptPassword(password, decrypted_pwd);
    if (!dec_res.ok) {
        result.code = dec_res.code;
        result.err = dec_res.err;
        return result;
    }

    /*生成密码哈希*/
    auto ph = CIM::util::Password::Hash(decrypted_pwd);
    if (ph.empty()) {
        result.err = "密码哈希生成失败！";
        return result;
    }

    /*创建用户*/
    CIM::dao::User u;
    u.nickname = nickname;
    u.mobile = mobile;
    u.password_hash = ph;

    if (!CIM::dao::UserDAO::Create(u, u.id, &err)) {
        CIM_LOG_ERROR(g_logger) << "Register Create user failed, mobile=" << mobile
                                << ", err=" << err;
        result.code = 500;
        result.err = "创建用户失败！";
        return result;
    }

    result.ok = true;
    result.data = std::move(u);
    return result;
}

UserResult UserService::Authenticate(const std::string& mobile, const std::string& password,
                                     const std::string& platform) {
    UserResult result;
    std::string err;

    /*密码解密*/
    std::string decrypted_pwd;
    auto dec_res = CIM::DecryptPassword(password, decrypted_pwd);
    if (!dec_res.ok) {
        result.code = dec_res.code;
        result.err = dec_res.err;
        return result;
    }

    /*获取用户信息*/
    if (!CIM::dao::UserDAO::GetByMobile(mobile, result.data, &err)) {
        CIM_LOG_ERROR(g_logger) << "Authenticate GetByMobile failed, mobile=" << mobile
                                << ", err=" << err;
        result.code = 404;
        result.err = "手机号或密码错误！";
        return result;
    }

    /*验证密码*/
    if (!CIM::util::Password::Verify(decrypted_pwd, result.data.password_hash)) {
        result.err = "手机号或密码错误";
        return result;
    }

    /*检查用户状态*/
    if (result.data.status != 1) {
        result.err = "账户被禁用!";
        return result;
    }

    result.ok = true;
    return result;
}

UserResult UserService::Forget(const std::string& mobile, const std::string& new_password) {
    UserResult result;
    std::string err;

    /*密码解密*/
    // Base64 解码
    std::string cipher_bin = CIM::base64decode(new_password);
    if (cipher_bin.empty()) {
        result.err = "密码解码失败！";
        return result;
    }

    // 私钥解密
    auto cm = CIM::CryptoModule::Get();
    if (!cm || !cm->isReady()) {
        result.err = "密钥模块未加载！";
        return result;
    }
    std::string decrypted_pwd = "";
    if (!cm->PrivateDecrypt(cipher_bin, decrypted_pwd)) {
        result.err = "密码解密失败！";
        return result;
    }

    /*检查手机号是否存在*/
    if (!CIM::dao::UserDAO::GetByMobile(mobile, result.data, &err)) {
        CIM_LOG_ERROR(g_logger) << "Forget GetByMobile failed, mobile=" << mobile
                                << ", err=" << err;
        result.code = 404;
        result.err = "手机号不存在！";
        return result;
    }

    /*生成新密码哈希*/
    auto ph = CIM::util::Password::Hash(decrypted_pwd);
    if (ph.empty()) {
        result.err = "密码哈希生成失败！";
        return result;
    }

    /*更新用户密码*/
    if (!CIM::dao::UserDAO::UpdatePassword(result.data.id, ph, &err)) {
        CIM_LOG_ERROR(g_logger) << "Forget UpdatePassword failed, mobile=" << mobile
                                << ", err=" << err;
        result.code = 500;
        result.err = "更新密码失败！";
        return result;
    }

    result.ok = true;
    return result;
}

ResultVoid UserService::LogLogin(const UserResult& user_result, const std::string& platform) {
    ResultVoid result;
    std::string err;

    CIM::dao::UserLoginLog log;
    log.user_id = user_result.data.id;
    log.platform = platform;
    log.login_at = TimeUtil::NowToS();
    log.status = user_result.ok ? 1 : 2;

    if (!CIM::dao::UserLoginLogDAO::Create(log, &err)) {
        CIM_LOG_ERROR(g_logger) << "LogLogin Create UserLoginLog failed, user_id="
                                << user_result.data.id << ", err=" << err;
        result.code = 500;
        result.err = "记录登录日志失败！";
        return result;
    }

    result.ok = true;
    return result;
}

ResultVoid UserService::GoOnline(const uint64_t id) {
    ResultVoid result;
    std::string err;

    if (!CIM::dao::UserDAO::UpdateOnlineStatus(id, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdateOnlineStatus failed, user_id=" << id << ", err=" << err;
        result.code = 500;
        result.err = "更新在线状态失败！";
        return result;
    }

    result.ok = true;
    return result;
}

ResultVoid UserService::Offline(const uint64_t id) {
    ResultVoid result;
    std::string err;

    if (!CIM::dao::UserDAO::UpdateOfflineStatus(id, &err)) {
        CIM_LOG_ERROR(g_logger) << "UpdateOfflineStatus failed, user_id=" << id << ", err=" << err;
        result.code = 500;
        result.err = "更新在线状态失败！";
        return result;
    }

    result.ok = true;
    return result;
}

StatusResult UserService::GetUserOnlineStatus(const uint64_t id) {
    StatusResult result;

    if (!CIM::dao::UserDAO::GetOnlineStatus(id, result.data, &result.err)) {
        CIM_LOG_ERROR(g_logger) << "GetUserOnlineStatus failed, user_id=" << id
                                << ", err=" << result.err;
        result.code = 404;
        result.err = "用户不存在！";
        return result;
    }

    result.ok = true;
    return result;
}

ResultVoid UserService::SaveConfigInfo(const uint64_t user_id, const std::string& theme_mode,
                                       const std::string& theme_bag_img,
                                       const std::string& theme_color,
                                       const std::string& notify_cue_tone,
                                       const std::string& keyboard_event_notify) {
    ResultVoid result;
    std::string err;

    CIM::dao::UserSettings new_settings;
    new_settings.user_id = user_id;
    new_settings.theme_mode = theme_mode;
    new_settings.theme_bag_img = theme_bag_img;
    new_settings.theme_color = theme_color;
    new_settings.notify_cue_tone = notify_cue_tone;
    new_settings.keyboard_event_notify = keyboard_event_notify;
    if (!CIM::dao::UserSettingsDAO::Upsert(new_settings, &err)) {
        CIM_LOG_ERROR(g_logger) << "Upsert new UserSettings failed, user_id=" << user_id
                                << ", err=" << err;
        result.code = 500;
        result.err = "保存用户设置失败！";
        return result;
    }
    result.ok = true;
    return result;
}

ConfigInfoResult UserService::LoadConfigInfo(const uint64_t user_id) {
    ConfigInfoResult result;
    std::string err;

    if (!CIM::dao::UserSettingsDAO::GetConfigInfo(user_id, result.data, &err)) {
        if (!err.empty()) {
            CIM_LOG_ERROR(g_logger)
                << "LoadConfigInfo failed, user_id=" << user_id << ", err=" << err;
            result.code = 500;
            result.err = "加载用户设置失败！";
            return result;
        }
    }

    result.ok = true;
    return result;
}

UserInfoResult UserService::LoadUserInfoSimple(const uint64_t uid) {
    UserInfoResult result;
    std::string err;

    if (!CIM::dao::UserDAO::GetUserInfoSimple(uid, result.data, &err)) {
        CIM_LOG_ERROR(g_logger) << "LoadUserInfoSimple failed, uid=" << uid << ", err=" << err;
        result.code = 404;
        result.err = "用户不存在！";
        return result;
    }

    result.ok = true;
    return result;
}

}  // namespace CIM::app