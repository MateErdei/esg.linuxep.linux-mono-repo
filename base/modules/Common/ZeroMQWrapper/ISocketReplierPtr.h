//
// Created by pair on 07/06/18.
//

#pragma once



#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {

        class ISocketReplier;
        using ISocketReplierPtr = std::unique_ptr<ISocketReplier>;
    }
}


