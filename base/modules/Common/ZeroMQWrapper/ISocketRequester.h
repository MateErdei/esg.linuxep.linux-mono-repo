//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETREQUESTER_H
#define EVEREST_BASE_ISOCKETREQUESTER_H

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

#endif //EVEREST_BASE_ISOCKETREQUESTER_H
