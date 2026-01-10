/**
 * @file talk_api_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_API_TALK_API_MODULE_HPP__
#define __IM_API_TALK_API_MODULE_HPP__

#include "infra/module/module.hpp"

#include "domain/service/message_service.hpp"
#include "domain/service/talk_service.hpp"
#include "domain/service/user_service.hpp"

namespace IM::api {

class TalkApiModule : public IM::Module {
   public:
    TalkApiModule(IM::domain::service::ITalkService::Ptr talk_service,
                  IM::domain::service::IUserService::Ptr user_service,
                  IM::domain::service::IMessageService::Ptr message_service);
    ~TalkApiModule() override = default;

    bool onServerReady() override;

   private:
    IM::domain::service::ITalkService::Ptr m_talk_service;
    IM::domain::service::IUserService::Ptr m_user_service;
    IM::domain::service::IMessageService::Ptr m_message_service;
};

}  // namespace IM::api

#endif  // __IM_API_TALK_API_MODULE_HPP__