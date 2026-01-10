/**
 * @file singleton.hpp
 * @brief 基础宏和基类
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 基础宏和基类。
 */

#ifndef __IM_BASE_SINGLETON_HPP__
#define __IM_BASE_SINGLETON_HPP__
#include <memory>

namespace IM {
template <class T, class X = void, int N = 0>
class Singleton {
   public:
    static T *GetInstance() {
        static T v;
        return &v;
    }
};

template <class T, class X = void, int N = 0>
class SingletonPtr {
   public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};
}  // namespace IM

#endif  // __IM_BASE_SINGLETON_HPP__