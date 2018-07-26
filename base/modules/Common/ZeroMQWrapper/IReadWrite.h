/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "IReadable.h"
#include "IWritable.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IReadWrite : public virtual IReadable, public virtual IWritable
        {
        };
    }
}

