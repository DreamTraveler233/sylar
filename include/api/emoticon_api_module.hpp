#ifndef __CIM_API_EMOTICON_API_MODULE_HPP__
#define __CIM_API_EMOTICON_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class EmoticonApiModule : public CIM::Module {
   public:
    EmoticonApiModule();
    ~EmoticonApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_EMOTICON_API_MODULE_HPP__