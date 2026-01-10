/**
 * @file upload_api_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_API_UPLOAD_API_MODULE_HPP__
#define __IM_API_UPLOAD_API_MODULE_HPP__

#include "core/net/http/multipart/multipart_parser.hpp"

#include "infra/module/module.hpp"

#include "domain/service/media_service.hpp"

namespace IM::api {

class UploadApiModule : public IM::Module {
   public:
    UploadApiModule(IM::domain::service::IMediaService::Ptr media_service,
                    IM::http::multipart::MultipartParser::ptr parser);
    ~UploadApiModule() override = default;

    bool onServerReady() override;

   private:
    IM::domain::service::IMediaService::Ptr m_media_service;
    IM::http::multipart::MultipartParser::ptr m_parser;
};

}  // namespace IM::api

#endif  // __IM_API_UPLOAD_API_MODULE_HPP__
