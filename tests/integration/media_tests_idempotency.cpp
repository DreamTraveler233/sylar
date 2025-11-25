#include "test_helpers.hpp"
#include <future>

int main() {
    namespace fs = std::filesystem;
    std::string work_dir = "test_data_idemp";
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

    uint64_t uid = 7777;
    std::string filename = "idemp.bin";
    uint64_t file_size = 2048;  // 2 parts
    auto init_res = svc.InitMultipartUpload(uid, filename, file_size);
    assert(init_res.ok);
    auto upload_id = init_res.data.upload_id;

    auto tmp1 = temp_base + "/id1.part";
    auto tmp1b = temp_base + "/id1_b.part";
    auto tmp2 = temp_base + "/id2.part";
    write_part_file(tmp1, 1024, 'Z');
    write_part_file(tmp1b, 1024, 'Z');
    write_part_file(tmp2, 1024, 'Y');

    // upload first part twice concurrently to check idempotency
    auto up_first = std::async(std::launch::async, [&svc, upload_id, tmp1]() {
        return svc.UploadPart(upload_id, 0, 2, tmp1);
    });
    auto up_first_dup = std::async(std::launch::async, [&svc, upload_id, tmp1b]() {
        return svc.UploadPart(upload_id, 0, 2, tmp1b);
    });
    auto r1 = up_first.get();
    auto r1dup = up_first_dup.get();
    assert(r1.ok);
    assert(r1dup.ok);

    // upload second part
    auto r2 = svc.UploadPart(upload_id, 1, 2, tmp2);
    assert(r2.ok && r2.data == true);

    IM::model::MediaFile media;
    auto gf = svc.GetMediaFileByUploadId(upload_id);
    assert(gf.ok);
    media = gf.data;
    assert(fs::exists(media.storage_path));

    fs::remove_all(work_dir);
    std::cout << "idempotency test passed" << std::endl;
    return 0;
}
