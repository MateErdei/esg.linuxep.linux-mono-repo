/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ReqRepTestImplementations.h"

namespace ReqRepTest
{
    Common::ZeroMQWrapper::IContextSharedPtr createContext()
    {
        std::cerr << "createContext from " << ::getpid() << std::endl;
        return Common::ZeroMQWrapper::createContext();
    }
} // namespace ReqRepTest
