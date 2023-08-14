// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "mount_monitor/mountinfo/IMountInfo.h"

namespace mount_monitor::mountinfoimpl
{
    class MountsList
    {
    public:
        void pushbackOrAssign(std::shared_ptr<mountinfo::IMountPoint>);
        
        std::string device(const std::string& mountPoint) const;
        
        mountinfo::IMountPointSharedVector mountsList();
        
    private:
        mountinfo::IMountPointSharedVector m_mountsList;
    };
}
