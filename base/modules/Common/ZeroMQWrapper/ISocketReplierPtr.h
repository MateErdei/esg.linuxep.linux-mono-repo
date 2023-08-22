/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common::ZeroMQWrapper
{
    class ISocketReplier;
    using ISocketReplierPtr = std::unique_ptr<ISocketReplier>;
} // namespace Common::ZeroMQWrapper
