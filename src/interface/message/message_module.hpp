/**
 * @file message_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_MESSAGE_MESSAGE_MODULE_HPP__
#define __IM_MESSAGE_MESSAGE_MODULE_HPP__

#include "infra/module/module.hpp"

#include "domain/service/message_service.hpp"

namespace IM::message {

class MessageModule : public IM::RockModule {
   public:
    explicit MessageModule(IM::domain::service::IMessageService::Ptr message_service);
    ~MessageModule() override = default;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

   private:
    IM::domain::service::IMessageService::Ptr m_message_service;
};

}  // namespace IM::message

#endif  // __IM_MESSAGE_MESSAGE_MODULE_HPP__
