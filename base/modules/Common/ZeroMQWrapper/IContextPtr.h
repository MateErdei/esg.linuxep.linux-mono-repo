//
// Created by pair on 08/06/18.
//

#pragma once


#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IContext;
        using IContextPtr = std::unique_ptr<IContext>;
    }
}


