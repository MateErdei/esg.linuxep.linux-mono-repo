/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReadWrite.h"
#include "ISocketSetup.h"

namespace Common::ZeroMQWrapper
{
    class ISocketReplier : public virtual IReadWrite, public virtual ISocketSetup
    {
    public:
    };
} // namespace Common::ZeroMQWrapper
