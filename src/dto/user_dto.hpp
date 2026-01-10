/**
 * @file user_dto.hpp
 * @brief 数据传输对象(DTO)
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 数据传输对象(DTO)。
 */

#ifndef __IM_DTO_USER_DTO_HPP__
#define __IM_DTO_USER_DTO_HPP__

#include <cstdint>
#include <string>

namespace IM::dto {

/**
 * @brief 用户信息传输对象 (用于展示用户资料)
 */
struct UserInfo {
    uint64_t uid = 0;           // 用户ID
    std::string nickname = "";  // 昵称
    std::string avatar = "";    // 头像URL
    std::string motto = "";     // 个性签名
    uint8_t gender = 0;         // 性别：0未知 1男 2女
    uint8_t is_qiye = 0;        // 是否企业用户：0=否 1=是
    std::string mobile = "";    // 手机号
    std::string email = "";     // 邮箱
};

}  // namespace IM::dto

#endif  // __IM_DTO_USER_DTO_HPP__
