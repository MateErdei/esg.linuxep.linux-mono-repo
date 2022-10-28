// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace common::signals
{
    class ISignalHandlerBase
    {
    public:
        virtual ~ISignalHandlerBase() = default;

        virtual bool triggered() = 0;
        virtual int monitorFd() = 0;
    };

    using ISignalHandlerSharedPtr = std::shared_ptr<ISignalHandlerBase>;
}

