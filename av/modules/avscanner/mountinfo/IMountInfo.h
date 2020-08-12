/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/mountinfo/IMountPoint.h"

#include <vector>

namespace avscanner::mountinfo
{
    using IMountPointSharedVector = std::vector<mountinfo::IMountPointSharedPtr>;

    class IMountInfo
    {

    public:
        inline IMountInfo() = default;

        inline virtual ~IMountInfo() = default;

        /**
         * Iterator for the list of mount points.
         */
        virtual IMountPointSharedVector mountPoints() = 0;

    };

    using IMountInfoSharedPtr = std::shared_ptr<IMountInfo>;
}