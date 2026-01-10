/**
 * @file noncopyable.hpp
 * @brief 基础宏和基类
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 基础宏和基类。
 */

#ifndef __IM_BASE_NONCOPYABLE_HPP__
#define __IM_BASE_NONCOPYABLE_HPP__

namespace IM {
class Noncopyable {
   public:
    Noncopyable(const Noncopyable &) = delete;
    void operator=(const Noncopyable &) = delete;

   protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};
}  // namespace IM

#endif  // __IM_BASE_NONCOPYABLE_HPP__