//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETREPLIER_H
#define EVEREST_BASE_ISOCKETREPLIER_H

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

#endif //EVEREST_BASE_ISOCKETREPLIER_H
