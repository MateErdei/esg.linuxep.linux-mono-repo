/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "SocketImpl.h"

#include <Common/ZeroMQWrapper/ISocketReplier.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class SocketReplierImpl :
                public SocketImpl,
                virtual public Common::ZeroMQWrapper::ISocketReplier
        {
        public:
            explicit SocketReplierImpl(ContextHolderSharedPtr context);

            std::vector<std::string> read() override;

            void write(const std::vector<std::string> &data) override;
        };
    }
}


