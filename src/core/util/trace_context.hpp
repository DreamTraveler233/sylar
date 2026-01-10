/**
 * @file trace_context.hpp
 * @brief 通用工具类
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 通用工具类。
 */

#ifndef __IM_UTIL_TRACE_CONTEXT_HPP__
#define __IM_UTIL_TRACE_CONTEXT_HPP__

#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace IM {

class TraceContext {
   public:
    static std::string GetTraceId();
    static void SetTraceId(const std::string &traceId);
    static void Clear();
    static std::string GenerateTraceId();
};

class TraceGuard {
   public:
    TraceGuard(const std::string &traceId) { TraceContext::SetTraceId(traceId); }
    ~TraceGuard() { TraceContext::Clear(); }
};

}  // namespace IM

#endif  // __IM_UTIL_TRACE_CONTEXT_HPP__
