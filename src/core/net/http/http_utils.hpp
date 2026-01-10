/**
 * @file http_utils.hpp
 * @brief 网络通信相关
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 网络通信相关。
 */

#ifndef __IM_NET_HTTP_HTTP_UTILS_HPP__
#define __IM_NET_HTTP_HTTP_UTILS_HPP__

#include <jsoncpp/json/json.h>
#include <string>

#include "core/net/http/http.hpp"

#include "common/result.hpp"

namespace IM::http {

using UidResult = IM::Result<uint64_t>;

/* 构造成功响应的JSON字符串 */
std::string Ok(const Json::Value &data = Json::Value(Json::objectValue));

/* 构造错误响应的JSON字符串 */
std::string Error(int code, const std::string &msg);

/* 解析请求体中的JSON字符串 */
bool ParseBody(const std::string &body, Json::Value &out);

// 从请求中提取 uid
UidResult GetUidFromToken(IM::http::HttpRequest::ptr req, IM::http::HttpResponse::ptr res);

// 根据错误码返回对应的 HTTP 状态码
IM::http::HttpStatus ToHttpStatus(const int code);

}  // namespace IM::http

#endif
