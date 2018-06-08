//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_ISOCKETREQUESTERPTR_H
#define EVEREST_BASE_ISOCKETREQUESTERPTR_H

#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {

        class ISocketRequester;
        using ISocketRequesterPtr = std::unique_ptr<ISocketRequester>;
    }
}
#endif //EVEREST_BASE_ISOCKETREQUESTERPTR_H
