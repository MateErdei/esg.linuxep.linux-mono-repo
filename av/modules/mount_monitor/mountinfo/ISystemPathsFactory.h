/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemPaths.h"

namespace mount_monitor::mountinfo
{
    class ISystemPathsFactory
    {
    public:
        virtual ~ISystemPathsFactory() = default;
        [[nodiscard]] virtual ISystemPathsSharedPtr createSystemPaths() const = 0;
    };

    using ISystemPathsFactorySharedPtr = std::shared_ptr<ISystemPathsFactory>;
}
