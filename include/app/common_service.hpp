#ifndef __IM_APP_COMMON_SERVICE_HPP__
#define __IM_APP_COMMON_SERVICE_HPP__

#include <string>

#include "dao/sms_verify_code_dao.hpp"
#include "dao/email_verify_code_dao.hpp"
#include "http/http_session.hpp"
#include "result.hpp"

namespace IM::app {

class CommonService {
   public:
    // 发送短信验证码
    static SmsCodeResult SendSmsCode(const std::string& mobile, const std::string& channel,
                                     IM::http::HttpSession::ptr session);

    // 验证短信验证码
    static SmsCodeResult VerifySmsCode(const std::string& mobile, const std::string& code,
                                       const std::string& channel);

    // 发送邮箱验证码
    static SmsCodeResult SendEmailCode(const std::string& email, const std::string& channel,
                                       IM::http::HttpSession::ptr session);

    // 验证邮箱验证码
    static SmsCodeResult VerifyEmailCode(const std::string& email, const std::string& code,
                                         const std::string& channel);

    // 初始化验证码清理定时器（幂等）
    static void InitCleanupTimer();

    // 初始化无效验证码删除定时器
    static void InitInvalidCodeCleanupTimer();

   private:
    // 实际发送短信
    static bool SendRealSms(const std::string& mobile, const std::string& sms_code,
                            const std::string& channel, std::string* err);

    // 实际发送邮件
    static bool SendRealEmail(const std::string& email, const std::string& title,
                              const std::string& body, std::string* err);

    // 阿里云短信发送
    static bool SendSmsViaAliyun(const std::string& mobile, const std::string& sms_code,
                                 const std::string& channel, std::string* err);

    // 腾讯云短信发送
    static bool SendSmsViaTencent(const std::string& mobile, const std::string& sms_code,
                                  const std::string& channel, std::string* err);
};

}  // namespace IM::app

#endif // __IM_APP_COMMON_SERVICE_HPP__