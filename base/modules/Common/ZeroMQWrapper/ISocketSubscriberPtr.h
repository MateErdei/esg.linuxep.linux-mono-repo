/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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


