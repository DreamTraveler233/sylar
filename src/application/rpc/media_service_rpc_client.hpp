/**
 * @file media_service_rpc_client.hpp
 * @brief RPC客户端实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 RPC客户端实现。
 */

#ifndef __IM_APPLICATION_RPC_MEDIA_SERVICE_RPC_CLIENT_HPP__
#define __IM_APPLICATION_RPC_MEDIA_SERVICE_RPC_CLIENT_HPP__

#include <atomic>
#include <jsoncpp/json/json.h>
#include <string>
#include <unordered_map>

#include "core/config/config.hpp"
#include "core/io/lock.hpp"
#include "core/net/rock/rock_stream.hpp"

#include "domain/service/media_service.hpp"

namespace IM::app::rpc {

class MediaServiceRpcClient : public IM::domain::service::IMediaService {
   public:
    MediaServiceRpcClient();

    Result<IM::model::UploadSession> InitMultipartUpload(const uint64_t user_id, const std::string &file_name,
                                                         const uint64_t file_size) override;

    Result<bool> UploadPart(const std::string &upload_id, const uint32_t split_index, const uint32_t split_num,
                            const std::string &temp_file_path) override;

    Result<IM::model::MediaFile> UploadFile(const uint64_t user_id, const std::string &file_name,
                                            const std::string &data) override;

    Result<IM::model::MediaFile> GetMediaFile(const std::string &media_id) override;

    Result<IM::model::MediaFile> GetMediaFileByUploadId(const std::string &upload_id) override;

    void InitTempCleanupTimer() override;

    std::string GetUploadTempPath(const std::string &upload_id) override;

   private:
    std::string GetStoragePath(const std::string &file_name) override;

    std::string GetTempPath(const std::string &upload_id) override;

    Result<IM::model::MediaFile> MergeParts(const IM::model::UploadSession &session) override;

   private:
    IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd, const Json::Value &body,
                                        uint32_t timeout_ms);

    std::string resolveSvcMediaAddr();

    bool parseUploadSession(const Json::Value &j, IM::model::UploadSession &out);

    bool parseMediaFile(const Json::Value &j, IM::model::MediaFile &out);

   private:
    IM::ConfigVar<std::string>::ptr m_rpc_addr;

    IM::RWMutex m_mutex;
    std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
    std::atomic<uint32_t> m_sn{1};
};

}  // namespace IM::app::rpc

#endif  // __IM_APPLICATION_RPC_MEDIA_SERVICE_RPC_CLIENT_HPP__
