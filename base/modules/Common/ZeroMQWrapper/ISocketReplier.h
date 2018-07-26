//
// Created by pair on 06/06/18.
//

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


