#pragma once

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <memory>

#include "app/media_service_impl.hpp"
#include "config/config.hpp"
#include "domain/repository/media_repository.hpp"
#include "infra/storage/istorage.hpp"

class MockMediaRepository : public IM::domain::repository::IMediaRepository {
   public:
    bool CreateMediaFile(const IM::model::MediaFile& f, std::string* err = nullptr) override {
        files_[f.id] = f;
        files_by_upload_[f.upload_id] = f;
        return true;
    }
    bool GetMediaFileByUploadId(const std::string& upload_id, IM::model::MediaFile& out,
                                std::string* err = nullptr) override {
        auto it = files_by_upload_.find(upload_id);
        if (it == files_by_upload_.end()) {
            if (err) *err = "not found";
            return false;
        }
        out = it->second;
        return true;
    }
    bool GetMediaFileById(const std::string& id, IM::model::MediaFile& out,
                          std::string* err = nullptr) override {
        auto it = files_.find(id);
        if (it == files_.end()) {
            if (err) *err = "not found";
            return false;
        }
        out = it->second;
        return true;
    }
    bool CreateMediaSession(const IM::model::UploadSession& s,
                            std::string* err = nullptr) override {
        sessions_[s.upload_id] = s;
        return true;
    }
    bool GetMediaSessionByUploadId(const std::string& upload_id, IM::model::UploadSession& out,
                                   std::string* err = nullptr) override {
        auto it = sessions_.find(upload_id);
        if (it == sessions_.end()) {
            if (err) *err = "not found";
            return false;
        }
        out = it->second;
        return true;
    }
    bool UpdateUploadedCount(const std::string& upload_id, uint32_t count,
                             std::string* err = nullptr) override {
        auto it = sessions_.find(upload_id);
        if (it == sessions_.end()) {
            if (err) *err = "not found";
            return false;
        }
        it->second.uploaded_count = count;
        return true;
    }
    bool UpdateMediaSessionStatus(const std::string& upload_id, uint8_t status,
                                  std::string* err = nullptr) override {
        auto it = sessions_.find(upload_id);
        if (it == sessions_.end()) {
            if (err) *err = "not found";
            return false;
        }
        it->second.status = status;
        return true;
    }

   private:
    std::unordered_map<std::string, IM::model::MediaFile> files_;
    std::unordered_map<std::string, IM::model::MediaFile> files_by_upload_;
    std::unordered_map<std::string, IM::model::UploadSession> sessions_;
};

static void write_part_file(const std::string& path, size_t size, char c) {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    for (size_t i = 0; i < size; ++i) ofs.put(c);
}
