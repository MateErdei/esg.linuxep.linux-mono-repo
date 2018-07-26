//
// Created by pair on 08/06/18.
//

#pragma once



#include <Common/ZeroMQWrapper/IIPCException.h>

#include <string>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQWrapperException
            : public Common::ZeroMQWrapper::IIPCException
        {
        public:
            explicit ZeroMQWrapperException(const std::string &message)
                    : Common::ZeroMQWrapper::IIPCException(message)
            {}
        };
    }
}



