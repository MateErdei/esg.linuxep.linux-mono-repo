//
// Created by pair on 08/06/18.
//

#pragma once


#include <Common/Exceptions/IException.h>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCException
            : public Common::Exceptions::IException
        {
        public:
            explicit IIPCException(const std::string& what)
                    : Common::Exceptions::IException(what)
            {}
        };
    }
}


