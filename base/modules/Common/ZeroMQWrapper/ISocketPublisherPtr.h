/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class ISocketPublisher;
        using ISocketPublisherPtr = std::unique_ptr<ISocketPublisher>;
    } // namespace ZeroMQWrapper
} // namespace Common
