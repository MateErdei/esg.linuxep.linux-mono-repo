//
// Created by pair on 07/06/18.
//

#pragma once



#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketPublisher;
        using ISocketPublisherPtr = std::unique_ptr<ISocketPublisher>;
    }
}


