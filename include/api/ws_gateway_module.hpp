#ifndef __IM_API_WS_GATEWAY_MODULE_HPP__
#define __IM_API_WS_GATEWAY_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class WsGatewayModule : public IM::Module {
   public:
    WsGatewayModule();
    ~WsGatewayModule() override = default;

    bool onServerReady() override;

    // 主动推送通用事件到指定用户的所有在线连接
    // event: 事件名（如 "im.message"）
    // payload: 事件数据（JSON对象）
    // ackid: 可选去重/确认ID（前端会自动ack）
    static void PushToUser(uint64_t uid, const std::string& event,
                           const Json::Value& payload = Json::Value(),
                           const std::string& ackid = "");

    // 主动推送一条 IM 消息事件
    // talk_mode: 1=单聊 2=群聊（当前实现重点覆盖单聊）
    // to_from_id: 单聊=对端用户ID，群聊=群ID
    // from_id: 发送者用户ID
    // body: 与 REST 返回一致的消息体（msg_id/sequence/msg_type/.../extra/quote）
    static void PushImMessage(uint8_t talk_mode, uint64_t to_from_id, uint64_t from_id,
                              const Json::Value& body);
};

}  // namespace IM::api

#endif // __IM_API_WS_GATEWAY_MODULE_HPP__
