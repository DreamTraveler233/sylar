/**
 * @file static_file_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_API_STATIC_FILE_MODULE_HPP__
#define __IM_API_STATIC_FILE_MODULE_HPP__

#include "infra/module/module.hpp"

namespace IM::api {

class StaticFileModule : public IM::Module {
   public:
    typedef std::shared_ptr<StaticFileModule> ptr;
    StaticFileModule();
    virtual bool onLoad() override;
    virtual bool onServerReady() override;
};

}  // namespace IM::api

#endif
