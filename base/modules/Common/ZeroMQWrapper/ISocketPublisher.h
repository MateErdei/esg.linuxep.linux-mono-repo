/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISocketSetup.h"
#include "IWritable.h"

namespace Common::ZeroMQWrapper
{
    class ISocketPublisher : public virtual IWritable, public virtual ISocketSetup
    {
    public:
    };
} // namespace Common::ZeroMQWrapper
