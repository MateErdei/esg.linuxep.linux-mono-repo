// Copyright 2023 Sophos Limited. All rights reserved.

#include "MountsList.h"

#include <algorithm>

void mount_monitor::mountinfoimpl::MountsList::pushbackOrAssign(std::shared_ptr<mountinfo::IMountPoint> mount)
{
    std::vector<std::shared_ptr<mountinfo::IMountPoint>>::iterator it = 
        std::find_if(m_mountsList.begin(), m_mountsList.end(), [mount](std::shared_ptr<mountinfo::IMountPoint> it) -> bool { return it->mountPoint() == mount->mountPoint(); });
    if (it != m_mountsList.end())
    {
        std::replace(m_mountsList.begin(), m_mountsList.end(), *it, mount);
    }
    else
    {
        m_mountsList.push_back(mount);
    }
}

std::string mount_monitor::mountinfoimpl::MountsList::device(const std::string& mountPoint) const
{
    for (const auto& it : m_mountsList)
    {
        if (it->mountPoint() == mountPoint)
        {
            return it->device();
        }
    }
    return "";
}

mount_monitor::mountinfo::IMountPointSharedVector mount_monitor::mountinfoimpl::MountsList::mountsList()
{
    return m_mountsList;
}
