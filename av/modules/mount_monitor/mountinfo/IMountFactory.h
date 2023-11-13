// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "mount_monitor/mountinfo/IMountInfo.h"

namespace mount_monitor::mountinfo
{
    class IMountFactory
    {
    public:
        virtual ~IMountFactory() = default;
        virtual IMountInfoSharedPtr newMountInfo() = 0;
    };

    using IMountFactorySharedPtr = std::shared_ptr<IMountFactory>;
}

