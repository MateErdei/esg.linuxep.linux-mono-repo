/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common
{
    namespace ZMQWrapperApi
    {
        class IContext;
        using IContextSharedPtr = std::shared_ptr<IContext>;
    } // namespace ZeroMQWrapper
} // namespace Common
