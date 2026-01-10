/**
 * @file media_file.hpp
 * @brief 数据模型实体
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 数据模型实体。
 */

#ifndef __IM_MODEL_MEDIA_FILE_HPP__
#define __IM_MODEL_MEDIA_FILE_HPP__

#include <cstdint>
#include <string>

namespace IM::model {

struct MediaFile {
    std::string id;
    std::string upload_id;
    uint64_t user_id = 0;
    std::string file_name;
    uint64_t file_size = 0;
    std::string mime;
    uint8_t storage_type = 1;
    std::string storage_path;
    std::string url;
    uint8_t status = 1;
    std::string created_at;
};

}  // namespace IM::model

#endif  // __IM_MODEL_MEDIA_FILE_HPP__