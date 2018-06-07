//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETSUBSCRIBER_H
#define EVEREST_BASE_ISOCKETSUBSCRIBER_H

#include "IReadable.h"
#include "ISocketSetup.h"

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketSubscriber : public virtual IReadable, public virtual ISocketSetup
        {
        public:
            virtual void subscribeTo(const std::string& subject) = 0;
        };
    }
}

#endif //EVEREST_BASE_ISOCKETSUBSCRIBER_H
