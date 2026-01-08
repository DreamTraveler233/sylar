#ifndef __IM_APP_RPC_USER_SERVICE_RPC_CLIENT_HPP__
#define __IM_APP_RPC_USER_SERVICE_RPC_CLIENT_HPP__

#include <atomic>
#include <string>
#include <unordered_map>

#include "core/io/lock.hpp"
#include "core/net/rock/rock_stream.hpp"
#include "core/config/config.hpp"
#include "domain/service/user_service.hpp"

namespace IM::app::rpc
{

    class UserServiceRpcClient : public IM::domain::service::IUserService
    {
    public:
        UserServiceRpcClient();
        ~UserServiceRpcClient() override = default;

        Result<IM::model::User> LoadUserInfo(const uint64_t uid) override;

        Result<void> UpdatePassword(const uint64_t uid, const std::string &old_password,
                                    const std::string &new_password) override;

        Result<void> UpdateUserInfo(const uint64_t uid, const std::string &nickname,
                                    const std::string &avatar, const std::string &motto,
                                    const uint32_t gender, const std::string &birthday) override;

        Result<void> UpdateMobile(const uint64_t uid, const std::string &password,
                                  const std::string &new_mobile, const std::string &sms_code) override;

        Result<void> UpdateEmail(const uint64_t uid, const std::string &password,
                                 const std::string &new_email, const std::string &email_code) override;

        Result<IM::model::User> GetUserByMobile(const std::string &mobile,
                                                const std::string &channel) override;

        Result<IM::model::User> GetUserByEmail(const std::string &email,
                                               const std::string &channel) override;

        Result<void> Offline(const uint64_t id) override;

        Result<std::string> GetUserOnlineStatus(const uint64_t id) override;

        Result<void> SaveConfigInfo(const uint64_t user_id, const std::string &theme_mode,
                                    const std::string &theme_bag_img, const std::string &theme_color,
                                    const std::string &notify_cue_tone,
                                    const std::string &keyboard_event_notify) override;

        Result<IM::model::UserSettings> LoadConfigInfo(const uint64_t user_id) override;

        Result<IM::dto::UserInfo> LoadUserInfoSimple(const uint64_t uid) override;

        Result<IM::model::User> Authenticate(const std::string &mobile, const std::string &password,
                                             const std::string &platform) override;

        Result<void> LogLogin(const Result<IM::model::User> &result, const std::string &platform,
                              IM::http::HttpSession::ptr session) override;

        Result<void> GoOnline(const uint64_t id) override;

        Result<IM::model::User> Register(const std::string &nickname, const std::string &mobile,
                                         const std::string &password, const std::string &sms_code,
                                         const std::string &platform) override;

        Result<IM::model::User> Forget(const std::string &mobile, const std::string &new_password,
                                       const std::string &sms_code) override;

    private:
        IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd,
                                            const Json::Value &body, uint32_t timeout_ms);
        std::string resolveSvcUserAddr();

        static bool parseUser(const Json::Value &j, IM::model::User &out);
        static bool parseUserInfo(const Json::Value &j, IM::dto::UserInfo &out);
        static bool parseUserSettings(const Json::Value &j, IM::model::UserSettings &out);

    private:
        IM::ConfigVar<std::string>::ptr m_rpc_addr;

        std::atomic<uint32_t> m_sn{1};
        IM::RWMutex m_mutex;
        std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
    };

} // namespace IM::app::rpc

#endif // __IM_APP_RPC_USER_SERVICE_RPC_CLIENT_HPP__
