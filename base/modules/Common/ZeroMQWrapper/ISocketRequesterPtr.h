/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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

