/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IMountPoint.h"

#include <vector>

namespace mount_monitor::mountinfo
{
    using IMountPointSharedVector = std::vector<mount_monitor::mountinfo::IMountPointSharedPtr>;

    class IMountInfo
    {

    public:
        inline IMountInfo() = default;

        inline virtual ~IMountInfo() = default;

        /**
         * @return Iterator for the list of mount points. Entries are std::shared_ptr<IMountPoint>
         */
        virtual IMountPointSharedVector mountPoints() = 0;

    };

    using IMountInfoSharedPtr = std::shared_ptr<IMountInfo>;
}