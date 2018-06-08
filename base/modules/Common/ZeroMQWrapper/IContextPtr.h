//
// Created by pair on 08/06/18.
//

#ifndef EVEREST_BASE_COMMON_ZEROMQ_WRAPPER_ICONTEXTPTR_H
#define EVEREST_BASE_COMMON_ZEROMQ_WRAPPER_ICONTEXTPTR_H

#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IContext;
        using IContextPtr = std::unique_ptr<IContext>;
    }
}

#endif //EVEREST_BASE_COMMON_ZEROMQ_WRAPPER_ICONTEXTPTR_H
