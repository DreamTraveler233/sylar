#include "interface/presence/presence_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/config/config.hpp"
#include "core/util/time_util.hpp"

#include "infra/db/redis.hpp"

#include "common/common.hpp"

namespace IM::presence {

static auto g_logger = IM_LOG_NAME("root");

static auto g_presence_redis_name =
    IM::Config::Lookup("presence.redis_name", std::string("default"), "presence redis name");
static auto g_presence_ttl_sec = IM::Config::Lookup("presence.ttl_sec", (uint32_t)120, "presence ttl seconds");
static auto g_presence_key_prefix =
    IM::Config::Lookup("presence.key_prefix", std::string("presence:"), "presence key prefix");

namespace {
constexpr uint32_t kCmdSetOnline = 201;
constexpr uint32_t kCmdSetOffline = 202;
constexpr uint32_t kCmdHeartbeat = 203;
constexpr uint32_t kCmdGetRoute = 204;

constexpr uint32_t kDefaultTtlSec = 120;

static bool RedisSetPresence(uint64_t uid, const std::string &gateway_rpc, uint32_t ttl_sec) {
    if (uid == 0 || gateway_rpc.empty()) {
        return false;
    }
    const auto key = g_presence_key_prefix->getValue() + std::to_string(uid);

    Json::Value v;
    v["gateway_rpc"] = gateway_rpc;
    v["last_seen_ms"] = (Json::UInt64)IM::TimeUtil::NowToMS();
    const auto value = IM::JsonUtil::ToString(v);

    auto r =
        IM::RedisUtil::Cmd(g_presence_redis_name->getValue(), "SET %s %s EX %u", key.c_str(), value.c_str(), ttl_sec);
    return r != nullptr;
}

static bool RedisDelPresence(uint64_t uid) {
    if (uid == 0) {
        return false;
    }
    const auto key = g_presence_key_prefix->getValue() + std::to_string(uid);
    auto r = IM::RedisUtil::Cmd(g_presence_redis_name->getValue(), "DEL %s", key.c_str());
    return r != nullptr;
}

static bool RedisGetPresence(uint64_t uid, std::string &gateway_rpc, uint64_t &last_seen_ms, int64_t &ttl_sec) {
    if (uid == 0) {
        return false;
    }
    const auto key = g_presence_key_prefix->getValue() + std::to_string(uid);

    gateway_rpc.clear();
    last_seen_ms = 0;
    ttl_sec = -1;

    {
        auto r = IM::RedisUtil::Cmd(g_presence_redis_name->getValue(), "GET %s", key.c_str());
        if (!r) {
            return false;
        }
        if (r->type == REDIS_REPLY_STRING && r->str) {
            const std::string raw(r->str, r->len);
            // 兼容两种格式：
            // 1) 旧：直接是 "ip:port"
            // 2) 新：json {gateway_rpc,last_seen_ms}
            if (!raw.empty() && raw.front() == '{') {
                Json::Value j;
                if (IM::JsonUtil::FromString(j, raw)) {
                    gateway_rpc = IM::JsonUtil::GetString(j, "gateway_rpc");
                    last_seen_ms = IM::JsonUtil::GetUint64(j, "last_seen_ms");
                }
            }
            if (gateway_rpc.empty()) {
                gateway_rpc = raw;
            }
        }
    }

    {
        auto r = IM::RedisUtil::Cmd(g_presence_redis_name->getValue(), "TTL %s", key.c_str());
        if (r && r->type == REDIS_REPLY_INTEGER) {
            ttl_sec = r->integer;
        }
    }

    return !gateway_rpc.empty();
}
}  // namespace

PresenceModule::PresenceModule() : RockModule("svc.presence", "0.1.0", "builtin") {}

bool PresenceModule::onServerUp() {
    registerService("rock", "im", "svc-presence");
    return true;
}

bool PresenceModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                       IM::RockStream::ptr /*stream*/) {
    const auto cmd = request ? request->getCmd() : 0;

    if (cmd != kCmdSetOnline && cmd != kCmdSetOffline && cmd != kCmdHeartbeat && cmd != kCmdGetRoute) {
        return false;
    }

    Json::Value body;
    if (!IM::JsonUtil::FromString(body, request->getBody())) {
        response->setResult(400);
        response->setResultStr("invalid json body");
        return true;
    }

    const uint64_t uid = IM::JsonUtil::GetUint64(body, "uid");
    const std::string gateway_rpc = IM::JsonUtil::GetString(body, "gateway_rpc");
    uint32_t ttl_sec = IM::JsonUtil::GetUint32(body, "ttl_sec");
    if (ttl_sec == 0) {
        ttl_sec = g_presence_ttl_sec->getValue();
    }

    if (uid == 0) {
        response->setResult(400);
        response->setResultStr("missing uid");
        return true;
    }

    if (cmd == kCmdSetOnline || cmd == kCmdHeartbeat) {
        if (gateway_rpc.empty()) {
            response->setResult(400);
            response->setResultStr("missing gateway_rpc");
            return true;
        }
        if (!RedisSetPresence(uid, gateway_rpc, ttl_sec)) {
            response->setResult(500);
            response->setResultStr("redis set failed");
            return true;
        }
        Json::Value out;
        out["uid"] = (Json::UInt64)uid;
        out["gateway_rpc"] = gateway_rpc;
        out["ttl_sec"] = (Json::UInt)ttl_sec;
        out["last_seen_ms"] = (Json::UInt64)IM::TimeUtil::NowToMS();
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdSetOffline) {
        if (!RedisDelPresence(uid)) {
            response->setResult(500);
            response->setResultStr("redis del failed");
            return true;
        }
        Json::Value out;
        out["uid"] = (Json::UInt64)uid;
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdGetRoute) {
        std::string gw;
        uint64_t last_seen_ms = 0;
        int64_t ttl = -1;
        const bool ok = RedisGetPresence(uid, gw, last_seen_ms, ttl);
        Json::Value out;
        out["uid"] = (Json::UInt64)uid;
        if (ok) {
            out["gateway_rpc"] = gw;
            if (last_seen_ms != 0) {
                out["last_seen_ms"] = (Json::UInt64)last_seen_ms;
            }
            if (ttl >= 0) {
                out["ttl_sec"] = (Json::Int64)ttl;
            }
            response->setResult(200);
            response->setResultStr("ok");
        } else {
            response->setResult(404);
            response->setResultStr("not found");
        }
        response->setBody(IM::JsonUtil::ToString(out));
        return true;
    }

    response->setResult(500);
    response->setResultStr("unhandled");
    return true;
}

bool PresenceModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

std::string PresenceModule::KeyForUid(uint64_t uid) {
    return std::string("presence:") + std::to_string(uid);
}

}  // namespace IM::presence
