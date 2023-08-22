/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common::ZeroMQWrapper
{
    class ISocketRequester;
    using ISocketRequesterPtr = std::unique_ptr<ISocketRequester>;
} // namespace Common::ZeroMQWrapper
