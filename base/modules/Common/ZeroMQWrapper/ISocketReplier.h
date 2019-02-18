/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReadWrite.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketReplier : public virtual IReadWrite, public virtual ISocketSetup
        {
        public:
        };
    } // namespace ZeroMQWrapper
} // namespace Common
