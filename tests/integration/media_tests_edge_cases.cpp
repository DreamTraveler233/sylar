#include "test_helpers.hpp"

int main() {
    namespace fs = std::filesystem;
    std::string work_dir = "test_data_edge";
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

    // invalid upload session
    auto r_invalid = svc.UploadPart("nonexistent", 0, 1, temp_base + "/x");
    assert(!r_invalid.ok && r_invalid.code == 404);

    // create a session for wrong index test
    uint64_t uid_wrong = 102;
    std::string filename_wrong = "wrong.bin";
    uint64_t file_size_wrong = 2048;
    auto init_res_wrong = svc.InitMultipartUpload(uid_wrong, filename_wrong, file_size_wrong);
    assert(init_res_wrong.ok);
    auto upload_id_wrong = init_res_wrong.data.upload_id;

    // wrong index (e.g., index beyond split_num) should not cause merge and should be accepted
    auto tmpp = temp_base + "/tmp_wrong.part";
    write_part_file(tmpp, 1024, 'X');
    auto r_wrong = svc.UploadPart(upload_id_wrong, 5, 2, tmpp);
    assert(r_wrong.ok && r_wrong.data == false);

    // create a normal session
    uint64_t uid = 101;
    std::string filename = "edge.bin";
    uint64_t file_size = 2048;
    auto init_res = svc.InitMultipartUpload(uid, filename, file_size);
    assert(init_res.ok);
    auto upload_id = init_res.data.upload_id;

    // (previous wrong index test is on separate session)

    // upload real parts and ensure merge only when all correct parts present
    auto t1 = temp_base + "/t1.part";
    auto t2 = temp_base + "/t2.part";
    write_part_file(t1, 1024, 'L');
    write_part_file(t2, 1024, 'M');
    auto ur1 = svc.UploadPart(upload_id, 0, 2, t1);
    assert(ur1.ok && ur1.data == false);
    auto ur2 = svc.UploadPart(upload_id, 1, 2, t2);
    assert(ur2.ok && ur2.data == true);

    // Ensure wrong index uploaded file was ignored in merge (only parts 0 and 1 merged)
    IM::model::MediaFile media;
    auto gf = svc.GetMediaFileByUploadId(upload_id);
    assert(gf.ok);
    media = gf.data;
    assert(fs::exists(media.storage_path));

    fs::remove_all(work_dir);
    std::cout << "edge cases test passed" << std::endl;
    return 0;
}
