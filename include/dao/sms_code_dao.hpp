#ifndef __CIM_DAO_SMS_CODE_DAO_HPP__
#define __CIM_DAO_SMS_CODE_DAO_HPP__

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace CIM::dao {

struct SmsCode {
    uint64_t id = 0;             // 主键ID
    std::string mobile = "";     // 手机号
    std::string channel = "";    // 场景（register/login/forget/update_mobile等）
    std::string code = "";       // 验证码
    uint8_t status = 1;          // 状态（1=未使用 2=已使用 3=已过期）
    std::string sent_ip = "";    // 发送请求IP
    std::time_t sent_at = 0;     // 发送时间
    std::time_t expire_at = 0;   // 过期时间
    std::time_t used_at = 0;     // 使用时间
    std::time_t created_at = 0;  // 创建时间
};

class SmsCodeDAO {
   public:
    // 创建新的短信验证码
    static bool Create(const SmsCode& code, std::string* err = nullptr);

    // 根据手机号和验证码验证
    static bool Verify(const std::string& mobile, const std::string& code,
                       const std::string& channel, std::string* err = nullptr);

    // 标记验证码为已使用
    static bool MarkAsUsed(const uint64_t id, std::string* err = nullptr);

    // 将过期验证码标记为失效
    static bool MarkExpiredAsInvalid(std::string* err = nullptr);

    // 删除失效验证码
    static bool DeleteInvalidCodes(std::string* err = nullptr);
};

}  // namespace CIM::dao

#endif // __CIM_DAO_SMS_CODE_DAO_HPP__