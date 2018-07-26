//
// Created by pair on 06/06/18.
//

#pragma once


#include "IWritable.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketPublisher : public virtual IWritable, public virtual ISocketSetup
        {
        public:
        };
    }
}


