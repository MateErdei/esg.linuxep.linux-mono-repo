/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IMountPoint.h"

#include <memory>
#include <vector>

class IMountInfo
{

public:
    inline IMountInfo() = default;

    inline virtual ~IMountInfo() = default;

    /**
     * Iterator for the list of mount points.
     */
    virtual std::vector<std::shared_ptr<IMountPoint>> mountPoints() = 0;

};