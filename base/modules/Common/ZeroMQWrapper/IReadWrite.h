//
// Created by pair on 07/06/18.
//

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

