//
// Created by pair on 11/06/18.
//

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


