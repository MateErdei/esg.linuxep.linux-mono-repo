/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common
{
    namespace ZMQWrapperApi
    {
        class IContext;
        using IContextPtr = std::unique_ptr<IContext>;
    } // namespace ZMQWrapperApi
} // namespace Common
