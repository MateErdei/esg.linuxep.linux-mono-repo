//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETREQUESTER_H
#define EVEREST_BASE_ISOCKETREQUESTER_H

#include "IReadWrite.h"
#include "ISocketSetup.h"

#include <memory>

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketRequester : public virtual IReadWrite, public virtual ISocketSetup
        {
        public:
        };

        using ISocketRequesterPtr = std::unique_ptr<ISocketRequester>;
    }
}

#endif //EVEREST_BASE_ISOCKETREQUESTER_H
