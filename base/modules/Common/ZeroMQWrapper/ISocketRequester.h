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
        class ISocketRequester : public virtual IReadWrite, public virtual ISocketSetup
        {
        };
    } // namespace ZeroMQWrapper
} // namespace Common
