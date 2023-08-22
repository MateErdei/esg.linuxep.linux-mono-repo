/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common::ZeroMQWrapper
{
    class ISocketPublisher;
    using ISocketPublisherPtr = std::unique_ptr<ISocketPublisher>;
} // namespace Common::ZeroMQWrapper
