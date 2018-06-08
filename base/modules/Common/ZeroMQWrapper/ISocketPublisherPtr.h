//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_ISOCKETPUBLISHERPTR_H
#define EVEREST_BASE_ISOCKETPUBLISHERPTR_H


#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketPublisher;
        using ISocketPublisherPtr = std::unique_ptr<ISocketPublisher>;
    }
}

#endif //EVEREST_BASE_ISOCKETPUBLISHERPTR_H
