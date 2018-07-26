//
// Created by pair on 06/06/18.
//

#pragma once


#include "IReadWrite.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketRequester : public virtual IReadWrite, public virtual ISocketSetup
        {
        public:
        };
    }
}


