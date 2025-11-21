#ifndef __IM_OTHER_LIBRARY_HPP__
#define __IM_OTHER_LIBRARY_HPP__

#include <memory>

#include "module.hpp"

namespace IM {
class Library {
   public:
    static Module::ptr GetModule(const std::string& path);
};

}  // namespace IM

#endif // __IM_OTHER_LIBRARY_HPP__
