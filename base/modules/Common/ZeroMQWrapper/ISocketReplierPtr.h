/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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


