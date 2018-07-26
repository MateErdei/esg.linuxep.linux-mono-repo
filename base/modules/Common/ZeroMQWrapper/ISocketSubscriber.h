/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "IReadable.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketSubscriber : public virtual IReadable, public virtual ISocketSetup
        {
        public:
            virtual void subscribeTo(const std::string& subject) = 0;
        };
    }
}


