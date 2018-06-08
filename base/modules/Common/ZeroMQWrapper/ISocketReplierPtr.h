//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_ISOCKETREPLIERPTR_H
#define EVEREST_BASE_ISOCKETREPLIERPTR_H


#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {

        class ISocketReplier;
        using ISocketReplierPtr = std::unique_ptr<ISocketReplier>;
    }
}

#endif //EVEREST_BASE_ISOCKETREPLIERPTR_H
