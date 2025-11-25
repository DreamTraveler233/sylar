#include "test_helpers.hpp"

int main() {
    namespace fs = std::filesystem;
    std::string work_dir = "test_data_basic";
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

    uint64_t uid = 1234;
    std::string filename = "test.bin";
    uint64_t file_size = 2048;  // 2KB
    auto init_res = svc.InitMultipartUpload(uid, filename, file_size);
    assert(init_res.ok);
    auto upload_id = init_res.data.upload_id;

    auto tmp1 = temp_base + "/tmp_part1.part";
    auto tmp2 = temp_base + "/tmp_part2.part";
    write_part_file(tmp1, 1024, 'A');
    write_part_file(tmp2, 1024, 'B');

    auto up1 = svc.UploadPart(upload_id, 0, 2, tmp1);
    assert(up1.ok && up1.data == false);
    auto up2 = svc.UploadPart(upload_id, 1, 2, tmp2);
    assert(up2.ok && up2.data == true);

    IM::model::MediaFile media;
    auto gf = svc.GetMediaFileByUploadId(upload_id);
    assert(gf.ok);
    media = gf.data;
    assert(fs::exists(media.storage_path));

    fs::remove_all(work_dir);
    std::cout << "basic test passed" << std::endl;
    return 0;
}
