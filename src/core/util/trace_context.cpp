#include "core/util/trace_context.hpp"

#include <iomanip>
#include <random>
#include <sstream>

#include "core/io/coroutine.hpp"

namespace IM {

std::string TraceContext::GetTraceId() {
    auto f = Coroutine::GetThis();
    if (f) {
        return f->getTraceId();
    }
    return "";
}

void TraceContext::SetTraceId(const std::string &traceId) {
    auto f = Coroutine::GetThis();
    if (f) {
        f->setTraceId(traceId);
    }
}

void TraceContext::Clear() {
    auto f = Coroutine::GetThis();
    if (f) {
        f->setTraceId("");
    }
}

std::string TraceContext::GenerateTraceId() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t id1 = dis(gen);
    uint64_t id2 = dis(gen);
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << id1 << std::setw(16) << std::setfill('0') << id2;
    return ss.str();
}

}  // namespace IM
