/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "ISocketSetup.h"
#include "IReadWrite.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketReplier : public virtual IReadWrite, public virtual ISocketSetup
        {
        public:
        };
    }
}


