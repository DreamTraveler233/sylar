#include "interface/media/media_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/util/hash_util.hpp"
#include "core/util/json_util.hpp"

namespace IM::media {

static auto g_logger = IM_LOG_NAME("root");

namespace {

constexpr uint32_t kCmdInitMultipartUpload = 801;
constexpr uint32_t kCmdUploadPart = 802;
constexpr uint32_t kCmdUploadFile = 803;
constexpr uint32_t kCmdGetMediaFile = 804;
constexpr uint32_t kCmdGetMediaFileByUploadId = 805;

Json::Value UploadSessionToJson(const IM::model::UploadSession &s) {
    Json::Value j(Json::objectValue);
    j["upload_id"] = s.upload_id;
    j["user_id"] = (Json::UInt64)s.user_id;
    j["file_name"] = s.file_name;
    j["file_size"] = (Json::UInt64)s.file_size;
    j["shard_size"] = (Json::UInt)s.shard_size;
    j["shard_num"] = (Json::UInt)s.shard_num;
    j["uploaded_count"] = (Json::UInt)s.uploaded_count;
    j["status"] = (Json::UInt)s.status;
    j["temp_path"] = s.temp_path;
    j["created_at"] = s.created_at;
    return j;
}

Json::Value MediaFileToJson(const IM::model::MediaFile &m) {
    Json::Value j(Json::objectValue);
    j["id"] = m.id;
    j["upload_id"] = m.upload_id;
    j["user_id"] = (Json::UInt64)m.user_id;
    j["file_name"] = m.file_name;
    j["file_size"] = (Json::UInt64)m.file_size;
    j["mime"] = m.mime;
    j["storage_type"] = (Json::UInt)m.storage_type;
    j["storage_path"] = m.storage_path;
    j["url"] = m.url;
    j["status"] = (Json::UInt)m.status;
    j["created_at"] = m.created_at;
    return j;
}

}  // namespace

MediaModule::MediaModule(IM::domain::service::IMediaService::Ptr media_service)
    : RockModule("svc.media", "0.1.0", "builtin"), m_media_service(std::move(media_service)) {}

bool MediaModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                    IM::RockStream::ptr /*stream*/) {
    const auto cmd = request->getCmd();
    if (cmd < kCmdInitMultipartUpload || cmd > kCmdGetMediaFileByUploadId) {
        return false;
    }

    Json::Value body;
    if (!IM::JsonUtil::FromString(body, request->getBody()) || !body.isObject()) {
        response->setResult(400);
        response->setResultStr("invalid json body");
        return true;
    }

    auto set_error = [&](int code, const std::string &err) {
        response->setResult(code);
        response->setResultStr(err);
        Json::Value out(Json::objectValue);
        out["code"] = code;
        out["err"] = err;
        response->setBody(IM::JsonUtil::ToString(out));
    };

    if (!m_media_service) {
        set_error(503, "svc-media not ready");
        return true;
    }

    if (cmd == kCmdInitMultipartUpload) {
        const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
        const std::string file_name = IM::JsonUtil::GetString(body, "file_name");
        const uint64_t file_size = IM::JsonUtil::GetUint64(body, "file_size");
        if (user_id == 0 || file_name.empty() || file_size == 0) {
            set_error(400, "invalid params");
            return true;
        }

        auto r = m_media_service->InitMultipartUpload(user_id, file_name, file_size);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }

        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = UploadSessionToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdUploadPart) {
        const std::string upload_id = IM::JsonUtil::GetString(body, "upload_id");
        const uint32_t split_index = IM::JsonUtil::GetUint32(body, "split_index");
        const uint32_t split_num = IM::JsonUtil::GetUint32(body, "split_num");
        const std::string temp_file_path = IM::JsonUtil::GetString(body, "temp_file_path");
        if (upload_id.empty() || split_index == 0 || split_num == 0 || temp_file_path.empty()) {
            set_error(400, "invalid params");
            return true;
        }

        auto r = m_media_service->UploadPart(upload_id, split_index, split_num, temp_file_path);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }

        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = r.data;
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdUploadFile) {
        const uint64_t user_id = IM::JsonUtil::GetUint64(body, "user_id");
        const std::string file_name = IM::JsonUtil::GetString(body, "file_name");
        const std::string data_b64 = IM::JsonUtil::GetString(body, "data_b64");
        if (user_id == 0 || file_name.empty() || data_b64.empty()) {
            set_error(400, "invalid params");
            return true;
        }

        const std::string data = IM::base64decode(data_b64);
        auto r = m_media_service->UploadFile(user_id, file_name, data);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }

        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = MediaFileToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdGetMediaFile) {
        const std::string media_id = IM::JsonUtil::GetString(body, "media_id");
        if (media_id.empty()) {
            set_error(400, "missing media_id");
            return true;
        }
        auto r = m_media_service->GetMediaFile(media_id);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }

        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = MediaFileToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    if (cmd == kCmdGetMediaFileByUploadId) {
        const std::string upload_id = IM::JsonUtil::GetString(body, "upload_id");
        if (upload_id.empty()) {
            set_error(400, "missing upload_id");
            return true;
        }
        auto r = m_media_service->GetMediaFileByUploadId(upload_id);
        if (!r.ok) {
            set_error(r.code == 0 ? 500 : r.code, r.err);
            return true;
        }

        Json::Value out(Json::objectValue);
        out["code"] = 200;
        out["data"] = MediaFileToJson(r.data);
        response->setBody(IM::JsonUtil::ToString(out));
        response->setResult(200);
        response->setResultStr("ok");
        return true;
    }

    set_error(500, "unhandled");
    return true;
}

bool MediaModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::media
