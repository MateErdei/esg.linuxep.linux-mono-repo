// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "mount_monitor/mountinfo/IMountFactory.h"
#include "mount_monitor/mountinfo/ISystemPathsFactory.h"

namespace mount_monitor::mountinfoimpl
{
    class MountFactory : public mount_monitor::mountinfo::IMountFactory
    {
    public:
        explicit MountFactory(mount_monitor::mountinfo::ISystemPathsFactorySharedPtr pathsFactory);
        mountinfo::IMountInfoSharedPtr newMountInfo() override;

    private:
        mount_monitor::mountinfo::ISystemPathsFactorySharedPtr pathsFactory_;
    };
}
