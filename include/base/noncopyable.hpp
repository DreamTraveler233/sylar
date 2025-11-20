#ifndef __CIM_BASE_NONCOPYABLE_HPP__
#define __CIM_BASE_NONCOPYABLE_HPP__

namespace CIM {
class Noncopyable {
   public:
    Noncopyable(const Noncopyable&) = delete;
    void operator=(const Noncopyable&) = delete;

   protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};
}  // namespace CIM

#endif // __CIM_BASE_NONCOPYABLE_HPP__