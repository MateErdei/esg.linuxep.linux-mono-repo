/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ReqRepTestImplementations.h"

namespace ReqRepTest
{
    Common::ZMQWrapperApi::IContextSharedPtr createContext()
    {
        std::cerr << "createContext from " << ::getpid() << std::endl;
        return Common::ZMQWrapperApi::createContext();
    }
} // namespace ReqRepTest
