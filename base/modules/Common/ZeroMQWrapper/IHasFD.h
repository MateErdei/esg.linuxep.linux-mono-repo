/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace Common::ZeroMQWrapper
{
    class IHasFD
    {
    public:
        virtual ~IHasFD() = default;
        virtual int fd() = 0;
    };

    using IHasFDPtr = std::unique_ptr<IHasFD>;
} // namespace Common::ZeroMQWrapper
