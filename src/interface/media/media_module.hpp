/**
 * @file media_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_INTERFACE_MEDIA_MEDIA_MODULE_HPP__
#define __IM_INTERFACE_MEDIA_MEDIA_MODULE_HPP__

#include <memory>

#include "infra/module/module.hpp"

#include "domain/service/media_service.hpp"

namespace IM::media {

class MediaModule : public IM::RockModule {
   public:
    using Ptr = std::shared_ptr<MediaModule>;

    explicit MediaModule(IM::domain::service::IMediaService::Ptr media_service);

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

   private:
    IM::domain::service::IMediaService::Ptr m_media_service;
};

}  // namespace IM::media

#endif  // __IM_INTERFACE_MEDIA_MEDIA_MODULE_HPP__
