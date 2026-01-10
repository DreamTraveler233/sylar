#include "application/rpc/media_service_rpc_client.hpp"

#include "core/system/application.hpp"
#include "core/system/env.hpp"
#include "core/util/hash_util.hpp"
#include "core/util/json_util.hpp"

namespace IM::app::rpc {

namespace {

constexpr uint32_t kTimeoutMs = 5000;

constexpr uint32_t kCmdInitMultipartUpload = 801;
constexpr uint32_t kCmdUploadPart = 802;
constexpr uint32_t kCmdUploadFile = 803;
constexpr uint32_t kCmdGetMediaFile = 804;
constexpr uint32_t kCmdGetMediaFileByUploadId = 805;

}  // namespace

MediaServiceRpcClient::MediaServiceRpcClient()
    : m_rpc_addr(IM::Config::Lookup("media.rpc_addr", std::string(""), "svc-media rpc address ip:port")) {}

IM::RockResult::ptr MediaServiceRpcClient::rockJsonRequest(const std::string &ip_port, uint32_t cmd,
                                                           const Json::Value &body, uint32_t timeout_ms) {
    if (ip_port.empty()) {
        return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr, nullptr);
    }

    IM::RockConnection::ptr conn;
    {
        IM::RWMutex::ReadLock lock(m_mutex);
        auto it = m_conns.find(ip_port);
        if (it != m_conns.end() && it->second && it->second->isConnected()) {
            conn = it->second;
        }
    }

    if (!conn) {
        auto addr = IM::Address::LookupAny(ip_port);
        if (!addr) {
            return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr, nullptr);
        }
        IM::RockConnection::ptr new_conn(new IM::RockConnection);
        if (!new_conn->connect(addr)) {
            return std::make_shared<IM::RockResult>(IM::AsyncSocketStream::NOT_CONNECT, 0, nullptr, nullptr);
        }
        new_conn->start();
        {
            IM::RWMutex::WriteLock lock(m_mutex);
            m_conns[ip_port] = new_conn;
        }
        conn = new_conn;
    }

    IM::RockRequest::ptr req = std::make_shared<IM::RockRequest>();
    req->setSn(m_sn.fetch_add(1));
    req->setCmd(cmd);
    req->setBody(IM::JsonUtil::ToString(body));
    return conn->request(req, timeout_ms);
}

std::string MediaServiceRpcClient::resolveSvcMediaAddr() {
    const auto fixed = m_rpc_addr->getValue();
    if (!fixed.empty()) {
        return fixed;
    }

    if (auto sd = IM::Application::GetInstance()->getServiceDiscovery()) {
        std::unordered_map<std::string,
                           std::unordered_map<std::string, std::unordered_map<uint64_t, IM::ServiceItemInfo::ptr>>>
            infos;
        sd->listServer(infos);
        auto itD = infos.find("im");
        if (itD == infos.end()) {
            sd->queryServer("im", "svc-media");
            return "";
        }
        auto itS = itD->second.find("svc-media");
        if (itS == itD->second.end() || itS->second.empty()) {
            sd->queryServer("im", "svc-media");
            return "";
        }
        auto info = itS->second.begin()->second;
        return info ? info->getData() : std::string();
    }

    return "";
}

bool MediaServiceRpcClient::parseUploadSession(const Json::Value &j, IM::model::UploadSession &out) {
    if (!j.isObject()) return false;
    out.upload_id = IM::JsonUtil::GetString(j, "upload_id");
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.file_name = IM::JsonUtil::GetString(j, "file_name");
    out.file_size = IM::JsonUtil::GetUint64(j, "file_size");
    out.shard_size = IM::JsonUtil::GetUint32(j, "shard_size");
    out.shard_num = IM::JsonUtil::GetUint32(j, "shard_num");
    out.uploaded_count = IM::JsonUtil::GetUint32(j, "uploaded_count");
    out.status = IM::JsonUtil::GetUint8(j, "status");
    out.temp_path = IM::JsonUtil::GetString(j, "temp_path");
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    return !out.upload_id.empty();
}

bool MediaServiceRpcClient::parseMediaFile(const Json::Value &j, IM::model::MediaFile &out) {
    if (!j.isObject()) return false;
    out.id = IM::JsonUtil::GetString(j, "id");
    out.upload_id = IM::JsonUtil::GetString(j, "upload_id");
    out.user_id = IM::JsonUtil::GetUint64(j, "user_id");
    out.file_name = IM::JsonUtil::GetString(j, "file_name");
    out.file_size = IM::JsonUtil::GetUint64(j, "file_size");
    out.mime = IM::JsonUtil::GetString(j, "mime");
    out.storage_type = IM::JsonUtil::GetUint8(j, "storage_type");
    out.storage_path = IM::JsonUtil::GetString(j, "storage_path");
    out.url = IM::JsonUtil::GetString(j, "url");
    out.status = IM::JsonUtil::GetUint8(j, "status");
    out.created_at = IM::JsonUtil::GetString(j, "created_at");
    return !out.id.empty();
}

Result<IM::model::UploadSession> MediaServiceRpcClient::InitMultipartUpload(const uint64_t user_id,
                                                                            const std::string &file_name,
                                                                            const uint64_t file_size) {
    Result<IM::model::UploadSession> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["file_name"] = file_name;
    req["file_size"] = (Json::UInt64)file_size;

    const auto addr = resolveSvcMediaAddr();
    auto rr = rockJsonRequest(addr, kCmdInitMultipartUpload, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-media unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value rsp;
    if (!IM::JsonUtil::FromString(rsp, rr->response->getBody()) || !rsp.isObject()) {
        result.code = 502;
        result.err = "invalid svc-media response";
        return result;
    }

    IM::model::UploadSession s;
    if (!rsp.isMember("data") || !parseUploadSession(rsp["data"], s)) {
        result.code = 502;
        result.err = "invalid svc-media data";
        return result;
    }

    result.ok = true;
    result.data = std::move(s);
    return result;
}

Result<bool> MediaServiceRpcClient::UploadPart(const std::string &upload_id, const uint32_t split_index,
                                               const uint32_t split_num, const std::string &temp_file_path) {
    Result<bool> result;

    Json::Value req(Json::objectValue);
    req["upload_id"] = upload_id;
    req["split_index"] = (Json::UInt)split_index;
    req["split_num"] = (Json::UInt)split_num;
    req["temp_file_path"] = temp_file_path;

    const auto addr = resolveSvcMediaAddr();
    auto rr = rockJsonRequest(addr, kCmdUploadPart, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-media unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value rsp;
    if (!IM::JsonUtil::FromString(rsp, rr->response->getBody()) || !rsp.isObject()) {
        result.code = 502;
        result.err = "invalid svc-media response";
        return result;
    }

    if (!rsp.isMember("data")) {
        result.code = 502;
        result.err = "missing svc-media data";
        return result;
    }

    result.ok = true;
    result.data = rsp["data"].asBool();
    return result;
}

Result<IM::model::MediaFile> MediaServiceRpcClient::UploadFile(const uint64_t user_id, const std::string &file_name,
                                                               const std::string &data) {
    Result<IM::model::MediaFile> result;

    Json::Value req(Json::objectValue);
    req["user_id"] = (Json::UInt64)user_id;
    req["file_name"] = file_name;
    req["data_b64"] = IM::base64encode(data);

    const auto addr = resolveSvcMediaAddr();
    auto rr = rockJsonRequest(addr, kCmdUploadFile, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-media unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value rsp;
    if (!IM::JsonUtil::FromString(rsp, rr->response->getBody()) || !rsp.isObject()) {
        result.code = 502;
        result.err = "invalid svc-media response";
        return result;
    }

    IM::model::MediaFile mf;
    if (!rsp.isMember("data") || !parseMediaFile(rsp["data"], mf)) {
        result.code = 502;
        result.err = "invalid svc-media data";
        return result;
    }

    result.ok = true;
    result.data = std::move(mf);
    return result;
}

Result<IM::model::MediaFile> MediaServiceRpcClient::GetMediaFile(const std::string &media_id) {
    Result<IM::model::MediaFile> result;

    Json::Value req(Json::objectValue);
    req["media_id"] = media_id;

    const auto addr = resolveSvcMediaAddr();
    auto rr = rockJsonRequest(addr, kCmdGetMediaFile, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-media unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value rsp;
    if (!IM::JsonUtil::FromString(rsp, rr->response->getBody()) || !rsp.isObject()) {
        result.code = 502;
        result.err = "invalid svc-media response";
        return result;
    }

    IM::model::MediaFile mf;
    if (!rsp.isMember("data") || !parseMediaFile(rsp["data"], mf)) {
        result.code = 502;
        result.err = "invalid svc-media data";
        return result;
    }

    result.ok = true;
    result.data = std::move(mf);
    return result;
}

Result<IM::model::MediaFile> MediaServiceRpcClient::GetMediaFileByUploadId(const std::string &upload_id) {
    Result<IM::model::MediaFile> result;

    Json::Value req(Json::objectValue);
    req["upload_id"] = upload_id;

    const auto addr = resolveSvcMediaAddr();
    auto rr = rockJsonRequest(addr, kCmdGetMediaFileByUploadId, req, kTimeoutMs);
    if (!rr || !rr->response) {
        result.code = 503;
        result.err = "svc-media unavailable";
        return result;
    }
    if (rr->response->getResult() != 200) {
        result.code = rr->response->getResult();
        result.err = rr->response->getResultStr();
        return result;
    }

    Json::Value rsp;
    if (!IM::JsonUtil::FromString(rsp, rr->response->getBody()) || !rsp.isObject()) {
        result.code = 502;
        result.err = "invalid svc-media response";
        return result;
    }

    IM::model::MediaFile mf;
    if (!rsp.isMember("data") || !parseMediaFile(rsp["data"], mf)) {
        result.code = 502;
        result.err = "invalid svc-media data";
        return result;
    }

    result.ok = true;
    result.data = std::move(mf);
    return result;
}

void MediaServiceRpcClient::InitTempCleanupTimer() {
    // Cleanup is managed by svc_media; no-op on client side.
}

std::string MediaServiceRpcClient::GetUploadTempPath(const std::string &upload_id) {
    // Keep this a local computation so gateway can stage/move files without a remote call.
    // NOTE: This assumes gateway and svc_media share the same media.temp_base_dir.
    const auto base =
        IM::Config::Lookup<std::string>("media.temp_base_dir", std::string("data/uploads/tmp"))->getValue();
    const auto abs_base = IM::EnvMgr::GetInstance()->getAbsoluteWorkPath(base);
    if (upload_id.empty()) return abs_base;
    return abs_base + "/" + upload_id;
}

std::string MediaServiceRpcClient::GetStoragePath(const std::string &file_name) {
    const auto base = IM::Config::Lookup<std::string>("media.upload_base_dir", std::string("data/uploads"))->getValue();
    const auto abs_base = IM::EnvMgr::GetInstance()->getAbsoluteWorkPath(base);
    if (file_name.empty()) return abs_base;
    return abs_base + "/" + file_name;
}

std::string MediaServiceRpcClient::GetTempPath(const std::string &upload_id) {
    return GetUploadTempPath(upload_id);
}

Result<IM::model::MediaFile> MediaServiceRpcClient::MergeParts(const IM::model::UploadSession & /*session*/) {
    return Result<IM::model::MediaFile>::Error(500, "MergeParts is not supported in rpc client");
}

}  // namespace IM::app::rpc
