//
// Created by pair on 11/06/18.
//

#pragma once



#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQTimeoutException
                : public Common::ZeroMQWrapper::IIPCTimeoutException
        {
        public:
            explicit ZeroMQTimeoutException(const std::string &message)
                    : Common::ZeroMQWrapper::IIPCTimeoutException(message)
            {}
        };
    }
}



