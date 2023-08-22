/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReadWrite.h"
#include "ISocketSetup.h"

namespace Common::ZeroMQWrapper
{
    class ISocketRequester : public virtual IReadWrite, public virtual ISocketSetup
    {
    };
} // namespace Common::ZeroMQWrapper
