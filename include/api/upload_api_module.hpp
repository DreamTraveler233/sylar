#ifndef __IM_API_UPLOAD_API_MODULE_HPP__
#define __IM_API_UPLOAD_API_MODULE_HPP__

#include "domain/service/media_service.hpp"
#include "http/multipart/multipart_parser.hpp"
#include "other/module.hpp"

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
