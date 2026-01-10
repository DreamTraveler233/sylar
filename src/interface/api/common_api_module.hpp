/**
 * @file common_api_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_API_COMMON_API_MODULE_HPP__
#define __IM_API_COMMON_API_MODULE_HPP__

#include "infra/module/module.hpp"

#include "domain/service/common_service.hpp"
#include "domain/service/user_service.hpp"

namespace IM::api {

class CommonApiModule : public IM::Module {
   public:
    CommonApiModule(IM::domain::service::ICommonService::Ptr common_service,
                    IM::domain::service::IUserService::Ptr user_service);
    ~CommonApiModule() override = default;

    bool onServerReady() override;

   private:
    IM::domain::service::ICommonService::Ptr m_common_service;
    IM::domain::service::IUserService::Ptr m_user_service;
};

}  // namespace IM::api

#endif  // __IM_API_COMMON_API_MODULE_HPP__
