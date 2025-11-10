#ifndef __CIM_DAO_USER_LOGIN_LOG_DAO_HPP__
#define __CIM_DAO_USER_LOGIN_LOG_DAO_HPP__

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace CIM::dao {

struct UserLoginLog {
    uint64_t id = 0;              // 主键ID
    uint64_t user_id = 0;         // 用户ID
    std::string mobile = "";      // 尝试登录的手机号，可空
    std::string platform = "";    // 登录平台
    std::string ip = "";          // 登录IP
    std::string address = "";     // IP归属地，可空
    std::string user_agent = "";  // UA/设备指纹，可空
    int8_t success = 0;           // 是否成功：1=成功 0=失败
    std::string reason = "";      // 失败原因，可空
    std::time_t created_at = 0;   // 记录时间
};

class UserLoginLogDAO {
   public:
    // 记录登录日志
    static bool Create(const UserLoginLog& log, std::string* err = nullptr);
};

}  // namespace CIM::dao

#endif  // __CIM_DAO_USER_LOGIN_LOG_DAO_HPP__