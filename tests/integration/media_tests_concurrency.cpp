#include "test_helpers.hpp"

#include <thread>
#include <vector>

int main() {
    namespace fs = std::filesystem;
    std::string work_dir = "test_data_concurrency";
    fs::remove_all(work_dir);
    fs::create_directories(work_dir);
    std::string upload_base = work_dir + "/uploads";
    std::string temp_base = upload_base + "/tmp";
    fs::create_directories(temp_base);

    auto tmp_conf = IM::Config::Lookup<std::string>("media.temp_base_dir");
    if (tmp_conf) tmp_conf->setValue(temp_base);
    auto up_conf = IM::Config::Lookup<std::string>("media.upload_base_dir");
    if (up_conf) up_conf->setValue(upload_base);
    auto mem_threshold = IM::Config::Lookup<size_t>("media.multipart_memory_threshold");
    if (mem_threshold) mem_threshold->setValue((size_t)1024);
    auto shard_size_conf = IM::Config::Lookup<uint32_t>("media.shard_size_default");
    if (shard_size_conf) shard_size_conf->setValue((uint32_t)1024);

    auto mock_repo = std::make_shared<MockMediaRepository>();
    auto storage_adapter = IM::infra::storage::CreateLocalStorageAdapter();
    IM::app::MediaServiceImpl svc(mock_repo, storage_adapter);

    uint64_t uid = 5050;
    std::string filename = "concurrent.bin";
    uint64_t file_size = 4096;  // 4KB -> 4 parts of 1KB
    auto init_res = svc.InitMultipartUpload(uid, filename, file_size);
    assert(init_res.ok);
    auto upload_id = init_res.data.upload_id;
    uint32_t shard_num = init_res.data.shard_num;

    // create part files
    std::vector<std::string> part_paths;
    for (uint32_t i = 0; i < shard_num; ++i) {
        std::string p = temp_base + "/p" + std::to_string(i) + ".part";
        write_part_file(p, 1024, (char)('0' + i));
        part_paths.push_back(p);
    }

    // spawn threads to upload parts concurrently
    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < shard_num; ++i) {
        threads.emplace_back([i, &svc, upload_id, shard_num, &part_paths]() {
            auto res = svc.UploadPart(upload_id, i, shard_num, part_paths[i]);
            if (!res.ok) {
                std::cerr << "Upload part " << i << " failed: " << res.err << std::endl;
            }
        });
    }
    for (auto &t : threads) t.join();

    // Verify merged file exists
    IM::model::MediaFile media;
    auto gf = svc.GetMediaFileByUploadId(upload_id);
    assert(gf.ok);
    media = gf.data;
    assert(fs::exists(media.storage_path));

    fs::remove_all(work_dir);
    std::cout << "concurrency test passed" << std::endl;
    return 0;
}
