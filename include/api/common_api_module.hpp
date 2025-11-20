#ifndef __CIM_API_COMMON_API_MODULE_HPP__
#define __CIM_API_COMMON_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class CommonApiModule : public CIM::Module {
   public:
    CommonApiModule();
    ~CommonApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_COMMON_API_MODULE_HPP__
