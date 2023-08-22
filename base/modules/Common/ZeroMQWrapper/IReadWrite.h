/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IReadable.h"
#include "IWritable.h"

namespace Common::ZeroMQWrapper
{
    class IReadWrite : public virtual IReadable, public virtual IWritable
    {
    };
} // namespace Common::ZeroMQWrapper
