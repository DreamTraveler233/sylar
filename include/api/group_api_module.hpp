#ifndef __CIM_API_GROUP_API_MODULE_HPP__
#define __CIM_API_GROUP_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class GroupApiModule : public CIM::Module {
   public:
    GroupApiModule();
    ~GroupApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_GROUP_API_MODULE_HPP__