/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common::ZMQWrapperApi
{
    class IContext;
    using IContextSharedPtr = std::shared_ptr<IContext>;
} // namespace Common::ZMQWrapperApi
