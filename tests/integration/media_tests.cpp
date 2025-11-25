// Copied from src/tests/media_tests.cpp
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "app/media_service_impl.hpp"
#include "config/config.hpp"
#include "domain/repository/media_repository.hpp"
#include "infra/storage/istorage.hpp"

// Mock repository implementation for testing
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

int main() {
    namespace fs = std::filesystem;
    std::string work_dir = "test_data";
    fs::remove_all(work_dir);
    fs::create_directories(work_dir);
    std::string upload_base = work_dir + "/uploads";
    std::string temp_base = upload_base + "/tmp";
    fs::create_directories(temp_base);

    // set config values for upload and temp base dir
    auto tmp_conf = IM::Config::Lookup<std::string>("media.temp_base_dir");
    if (tmp_conf) tmp_conf->setValue(temp_base);
    auto up_conf = IM::Config::Lookup<std::string>("media.upload_base_dir");
    if (up_conf) up_conf->setValue(upload_base);
    // set memory threshold low for testing
    auto mem_threshold = IM::Config::Lookup<size_t>("media.multipart_memory_threshold");
    if (mem_threshold) mem_threshold->setValue((size_t)1024);  // 1KB so parser writes to temp file
    // set shard size so there will be 2 parts for our 2KB file
    auto shard_size_conf = IM::Config::Lookup<uint32_t>("media.shard_size_default");
    if (shard_size_conf) shard_size_conf->setValue((uint32_t)1024);

    // create mock repo and storage adapter
    auto mock_repo = std::make_shared<MockMediaRepository>();
    auto storage_adapter = IM::infra::storage::CreateLocalStorageAdapter();

    IM::app::MediaServiceImpl svc(mock_repo, storage_adapter);

    uint64_t uid = 1234;
    std::string filename = "test.bin";
    uint64_t file_size = 2048;  // 2KB
    auto init_res = svc.InitMultipartUpload(uid, filename, file_size);
    assert(init_res.ok);
    auto upload_id = init_res.data.upload_id;

    std::cout << "InitMultipartUpload upload_id=" << upload_id
              << " shard_size=" << init_res.data.shard_size << std::endl;

    // simulate two parts: split into 2 parts of 1KB each
    auto tmp1 = temp_base + "/tmp_part1.part";
    auto tmp2 = temp_base + "/tmp_part2.part";
    {
        std::ofstream ofs(tmp1, std::ios::binary | std::ios::trunc);
        for (int i = 0; i < 1024; ++i) ofs.put((char)('A' + (i % 26)));
    }
    {
        std::ofstream ofs(tmp2, std::ios::binary | std::ios::trunc);
        for (int i = 0; i < 1024; ++i) ofs.put((char)('a' + (i % 26)));
    }

    auto up1 = svc.UploadPart(upload_id, 0, 2, tmp1);
    if (!up1.ok) {
        std::cerr << "Upload Part 1 failed: " << up1.err << std::endl;
        return 2;
    }
    std::cout << "Uploaded part 1, completed=" << up1.data << std::endl;
    auto up2 = svc.UploadPart(upload_id, 1, 2, tmp2);
    if (!up2.ok) {
        std::cerr << "Upload Part 2 failed: " << up2.err << std::endl;
        return 3;
    }
    std::cout << "Uploaded part 2, completed=" << up2.data << std::endl;

    if (!up2.data) {
        std::cout << "Upload not reported as completed (unexpected)" << std::endl;
    }

    // After upload, check resultant MediaFile exists via repo
    IM::model::MediaFile media;
    auto gf = svc.GetMediaFileByUploadId(upload_id);
    if (gf.ok) {
        media = gf.data;
        std::cout << "Media created: id=" << media.id << " url=" << media.url
                  << " storage_path=" << media.storage_path << std::endl;
        assert(fs::exists(media.storage_path));
    } else {
        std::cerr << "GetMediaFileByUploadId failed: " << gf.err << std::endl;
        return 1;
    }

    // tidy up
    fs::remove_all(work_dir);
    std::cout << "All tests passed" << std::endl;
    return 0;
}
