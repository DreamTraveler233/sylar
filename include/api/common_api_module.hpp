#ifndef __IM_API_COMMON_API_MODULE_HPP__
#define __IM_API_COMMON_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class CommonApiModule : public IM::Module {
   public:
    CommonApiModule();
    ~CommonApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_COMMON_API_MODULE_HPP__
