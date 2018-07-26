//
// Created by pair on 07/06/18.
//

#pragma once


#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {

        class ISocketRequester;
        using ISocketRequesterPtr = std::unique_ptr<ISocketRequester>;
    }
}

