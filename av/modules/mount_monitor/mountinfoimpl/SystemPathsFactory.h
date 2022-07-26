/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "mount_monitor/mountinfo/ISystemPathsFactory.h"

#include <memory>

namespace mount_monitor::mountinfoimpl
{
    class SystemPathsFactory : public mount_monitor::mountinfo::ISystemPathsFactory
    {
    public:
        [[nodiscard]] mountinfo::ISystemPathsSharedPtr createSystemPaths() const override;
    };
}