#ifndef __CIM_API_CONTACT_API_MODULE_HPP__
#define __CIM_API_CONTACT_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class ContactApiModule : public CIM::Module {
   public:
    ContactApiModule();
    ~ContactApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_CONTACT_API_MODULE_HPP__