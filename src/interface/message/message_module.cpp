#include "interface/message/message_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/util/json_util.hpp"
#include "dto/message_dto.hpp"
#include "common/common.hpp"

namespace IM::message {

static auto g_logger = IM_LOG_NAME("root");

namespace {
constexpr uint32_t kCmdLoadRecords = 301;
constexpr uint32_t kCmdLoadHistoryRecords = 302;
constexpr uint32_t kCmdLoadForwardRecords = 303;
constexpr uint32_t kCmdDeleteMessages = 304;
constexpr uint32_t kCmdDeleteAllMessagesInTalkForUser = 305;
constexpr uint32_t kCmdClearTalkRecords = 306;
constexpr uint32_t kCmdRevokeMessage = 307;
constexpr uint32_t kCmdSendMessage = 308;
constexpr uint32_t kCmdUpdateMessageStatus = 309;

Json::Value MessageRecordToJson(const IM::dto::MessageRecord& r) {
    Json::Value root(Json::objectValue);
    root["msg_id"] = r.msg_id;
    root["sequence"] = (Json::UInt64)r.sequence;
    root["msg_type"] = r.msg_type;
    root["from_id"] = (Json::UInt64)r.from_id;
    root["nickname"] = r.nickname;
    root["avatar"] = r.avatar;
    root["is_revoked"] = (Json::UInt)r.is_revoked;
    root["status"] = (Json::UInt)r.status;
    root["send_time"] = r.send_time;
    root["extra"] = r.extra;
    root["quote"] = r.quote;
    return root;
}

Json::Value MessagePageToJson(const IM::dto::MessagePage& p) {
    Json::Value root(Json::objectValue);
    root["cursor"] = (Json::UInt64)p.cursor;
    Json::Value arr(Json::arrayValue);
    for (const auto& it : p.items) {
        arr.append(MessageRecordToJson(it));
    }
    root["items"] = arr;
    return root;
}

bool ParseMsgIds(const Json::Value& arr, std::vector<std::string>& out) {
    out.clear();
    if (!arr.isArray()) return false;
    for (auto& v : arr) {
        if (v.isString()) {
            out.push_back(v.asString());
        } else if (v.isUInt64()) {
            out.push_back(std::to_string(v.asUInt64()));
        } else if (v.isInt()) {
            out.push_back(std::to_string(v.asInt()));
        }
    }
    return true;
}

bool ParseMentionedUserIds(const Json::Value& arr, std::vector<uint64_t>& out) {
    out.clear();
    if (!arr.isArray()) return false;
    for (auto& v : arr) {
        if (v.isUInt64()) {
            out.push_back(v.asUInt64());
        } else if (v.isString()) {
            out.push_back(std::stoull(v.asString()));
        } else if (v.isInt()) {
            out.push_back((uint64_t)v.asInt64());
        }
    }
    return true;
}

}  // namespace

MessageModule::MessageModule(IM::domain::service::IMessageService::Ptr message_service)
    : RockModule("svc.message", "0.1.0", "builtin"), m_message_service(std::move(message_service)) {}

bool MessageModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                      IM::RockStream::ptr /*stream*/) {
    const auto cmd = request->getCmd();
    if (cmd < kCmdLoadRecords || cmd > kCmdUpdateMessageStatus) {
        return false;
    }

    Json::Value body;
    if (!IM::JsonUtil::FromString(body, request->getBody()) || !body.isObject()) {
        response->setResult(400);
        response->setResultStr("invalid json body");
        return true;
    }

    auto set_error = [&](int code, const std::string& err) {
        response->setResult(code);
        response->setResultStr(err);
        Json::Value out(Json::objectValue);
        out["code"] = code;
        out["err"] = err;
        response->setBody(IM::JsonUtil::ToString(out));
    };

    const uint64_t current_user_id = IM::JsonUtil::GetUint64(body, "current_user_id");

    if (cmd == kCmdSendMessage) {
        const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
        const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");
        const uint16_t msg_type = IM::JsonUtil::GetUint16(body, "msg_type");
        const std::string content_text = IM::JsonUtil::GetString(body, "content_text");
        const std::string extra = IM::JsonUtil::GetString(body, "extra");
        const std::string quote_msg_id = IM::JsonUtil::GetString(body, "quote_msg_id");
        const std::string msg_id = IM::JsonUtil::GetString(body, "msg_id");
        std::vector<uint64_t> mentioned_user_ids;
        if (body.isMember("mentioned_user_ids")) {
            (void)ParseMentionedUserIds(body["mentioned_user_ids"], mentioned_user_ids);
        }

        auto r = m_message_service->SendMessage(current_user_id, talk_mode, to_from_id, msg_type,
                                               content_text, extra, quote_msg_id, msg_id,
                                               mentioned_user_ids);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = MessageRecordToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    const uint8_t talk_mode = IM::JsonUtil::GetUint8(body, "talk_mode");
    const uint64_t to_from_id = IM::JsonUtil::GetUint64(body, "to_from_id");

    if (cmd == kCmdLoadRecords) {
        const uint64_t cursor = IM::JsonUtil::GetUint64(body, "cursor");
        const uint32_t limit = IM::JsonUtil::GetUint32(body, "limit");
        auto r = m_message_service->LoadRecords(current_user_id, talk_mode, to_from_id, cursor, limit);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = MessagePageToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdLoadHistoryRecords) {
        const uint16_t msg_type = IM::JsonUtil::GetUint16(body, "msg_type");
        const uint64_t cursor = IM::JsonUtil::GetUint64(body, "cursor");
        const uint32_t limit = IM::JsonUtil::GetUint32(body, "limit");
        auto r = m_message_service->LoadHistoryRecords(current_user_id, talk_mode, to_from_id, msg_type,
                                                      cursor, limit);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = MessagePageToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdLoadForwardRecords) {
        std::vector<std::string> msg_ids;
        if (!body.isMember("msg_ids") || !ParseMsgIds(body["msg_ids"], msg_ids)) {
            set_error(400, "msg_ids required");
            return true;
        }
        auto r = m_message_service->LoadForwardRecords(current_user_id, talk_mode, msg_ids);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        Json::Value out(Json::objectValue);
        out["code"] = 200;
        Json::Value arr(Json::arrayValue);
        for (auto& it : r.data) arr.append(MessageRecordToJson(it));
        out["data"] = arr;
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdDeleteMessages) {
        std::vector<std::string> msg_ids;
        if (!body.isMember("msg_ids") || !ParseMsgIds(body["msg_ids"], msg_ids)) {
            set_error(400, "msg_ids required");
            return true;
        }
        auto r = m_message_service->DeleteMessages(current_user_id, talk_mode, to_from_id, msg_ids);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        response->setResult(200);
        response->setResultStr("ok");
        response->setBody("{}");
        return true;
    }

    if (cmd == kCmdDeleteAllMessagesInTalkForUser) {
        auto r = m_message_service->DeleteAllMessagesInTalkForUser(current_user_id, talk_mode, to_from_id);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        response->setResult(200);
        response->setResultStr("ok");
        response->setBody("{}");
        return true;
    }

    if (cmd == kCmdClearTalkRecords) {
        auto r = m_message_service->ClearTalkRecords(current_user_id, talk_mode, to_from_id);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        response->setResult(200);
        response->setResultStr("ok");
        response->setBody("{}");
        return true;
    }

    if (cmd == kCmdRevokeMessage) {
        const std::string msg_id = IM::JsonUtil::GetString(body, "msg_id");
        auto r = m_message_service->RevokeMessage(current_user_id, talk_mode, to_from_id, msg_id);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        response->setResult(200);
        response->setResultStr("ok");
        response->setBody("{}");
        return true;
    }

    if (cmd == kCmdUpdateMessageStatus) {
        const std::string msg_id = IM::JsonUtil::GetString(body, "msg_id");
        const uint8_t status = IM::JsonUtil::GetUint8(body, "status");
        auto r = m_message_service->UpdateMessageStatus(current_user_id, talk_mode, to_from_id, msg_id,
                                                       status);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }
        response->setResult(200);
        response->setResultStr("ok");
        response->setBody("{}");
        return true;
    }

    IM_LOG_WARN(g_logger) << "unhandled cmd=" << cmd;
    return false;
}

bool MessageModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::message
