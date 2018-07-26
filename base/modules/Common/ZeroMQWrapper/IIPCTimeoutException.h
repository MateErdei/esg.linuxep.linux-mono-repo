/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once



#include "IIPCException.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCTimeoutException : public IIPCException
        {
        public:
            explicit IIPCTimeoutException(const std::string& what)
                    : IIPCException(what)
            {}
        };
    }
}


