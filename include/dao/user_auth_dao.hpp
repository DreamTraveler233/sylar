#ifndef __IM_DAO_USER_AUTH_DAO_HPP__
#define __IM_DAO_USER_AUTH_DAO_HPP__

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>
#include <memory>

#include "db/mysql.hpp"

namespace IM::dao {

struct UserAuth {
    uint64_t user_id = 0;                              // 用户ID (im_user.id)
    std::string password_hash;                         // 密码哈希（bcrypt/argon2 等）
    std::string password_algo = "PBKDF2-HMAC-SHA256";  // 哈希算法标识，默认为 PBKDF2-HMAC-SHA256
    int16_t password_version = 1;                      // 密码版本
    std::time_t last_reset_at = 0;                     // 最近重置时间，可空
    std::time_t created_at = 0;                        // 创建时间 (timestamp)
    std::time_t updated_at = 0;                        // 更新时间 (timestamp)
};

class UserAuthDao {
   public:
    // 创建用户认证记录
    static bool Create(const std::shared_ptr<IM::MySQL>& db, const UserAuth& ua,
                       std::string* err = nullptr);
    // 根据用户 ID 获取用户认证记录
    static bool GetByUserId(const uint64_t user_id, UserAuth& out, std::string* err = nullptr);
    // 更新用户密码哈希
    static bool UpdatePasswordHash(const uint64_t user_id, const std::string& new_password_hash,
                                   std::string* err = nullptr);
};

}  // namespace IM::dao

#endif  // __IM_DAO_USER_AUTH_DAO_HPP__
