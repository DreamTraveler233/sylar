/**
 * @file library.hpp
 * @brief 模块化机制
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 模块化机制。
 */

#ifndef __IM_INFRA_MODULE_LIBRARY_HPP__
#define __IM_INFRA_MODULE_LIBRARY_HPP__

#include <memory>

#include "module.hpp"

namespace IM {
class Library {
   public:
    static Module::ptr GetModule(const std::string &path);
};

}  // namespace IM

#endif  // __IM_INFRA_MODULE_LIBRARY_HPP__
