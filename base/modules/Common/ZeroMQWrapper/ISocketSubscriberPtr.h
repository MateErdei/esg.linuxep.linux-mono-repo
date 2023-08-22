/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common::ZeroMQWrapper
{
    class ISocketSubscriber;
    using ISocketSubscriberPtr = std::unique_ptr<ISocketSubscriber>;
} // namespace Common::ZeroMQWrapper
