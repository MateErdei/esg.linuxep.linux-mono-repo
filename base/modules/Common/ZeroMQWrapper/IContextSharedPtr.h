/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IContext;
        using IContextSharedPtr = std::shared_ptr<IContext>;
    } // namespace ZeroMQWrapper
} // namespace Common
