/**
 * @file presence_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_INTERFACE_PRESENCE_PRESENCE_MODULE_HPP__
#define __IM_INTERFACE_PRESENCE_PRESENCE_MODULE_HPP__

#include <cstdint>

#include "infra/module/module.hpp"

namespace IM::presence {

// Rock RPC commands for presence service
// 201: SetOnline
// 202: SetOffline
// 203: Heartbeat (refresh TTL)
// 204: GetRoute
class PresenceModule : public IM::RockModule {
   public:
    using ptr = std::shared_ptr<PresenceModule>;

    PresenceModule();
    ~PresenceModule() override = default;

    bool onServerUp() override;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;
    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

   private:
    static std::string KeyForUid(uint64_t uid);
};

}  // namespace IM::presence

#endif  // __IM_INTERFACE_PRESENCE_PRESENCE_MODULE_HPP__
