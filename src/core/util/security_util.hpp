/**
 * @file security_util.hpp
 * @brief 通用工具类
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 通用工具类。
 */

#ifndef __IM_UTIL_SECURITY_UTIL_HPP__
#define __IM_UTIL_SECURITY_UTIL_HPP__

#include <string>

#include "common/result.hpp"

namespace IM::util {

// 将密码解密成明文
Result<std::string> DecryptPassword(const std::string &encrypted_password, std::string &out_plaintext);

}  // namespace IM::util

#endif
