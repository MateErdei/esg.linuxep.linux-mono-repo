//
// Created by pair on 07/06/18.
//

#pragma once



#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketSubscriber;
        using ISocketSubscriberPtr = std::unique_ptr<ISocketSubscriber>;
    }
}


