//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_ISOCKETSUBSCRIBERPTR_H
#define EVEREST_BASE_ISOCKETSUBSCRIBERPTR_H


#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketSubscriber;
        using ISocketSubscriberPtr = std::unique_ptr<ISocketSubscriber>;
    }
}

#endif //EVEREST_BASE_ISOCKETSUBSCRIBERPTR_H
