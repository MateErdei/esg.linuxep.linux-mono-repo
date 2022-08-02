// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace common::signals
{
    class IMonitorable
    {
    public:
        virtual ~IMonitorable() = default;
        virtual int monitorFd() = 0;
        virtual void triggered() = 0;
    };

    using IMonitorablePtr = std::shared_ptr<IMonitorable>;
}
