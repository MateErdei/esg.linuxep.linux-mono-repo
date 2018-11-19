/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "SocketImpl.h"

#include <Common/ZeroMQWrapper/ISocketRequester.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketRequesterImpl : public SocketImpl, public virtual Common::ZeroMQWrapper::ISocketRequester
        {
        public:
            explicit SocketRequesterImpl(ContextHolderSharedPtr context);

            std::vector<std::string> read() override;

            void write(const std::vector<std::string> &data) override;

        };
    }
}


