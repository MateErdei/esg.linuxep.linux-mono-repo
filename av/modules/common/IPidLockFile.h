// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace common
{
    class IPidLockFile
    {
    public:
        virtual ~IPidLockFile() = default;
    };

    using IPidLockFileSharedPtr = std::shared_ptr<IPidLockFile>;
}