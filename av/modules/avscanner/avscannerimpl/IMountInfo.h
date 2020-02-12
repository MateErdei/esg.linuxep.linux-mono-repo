/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IMountPoint.h"

#include <vector>

class IMountInfo
{

public:
    inline IMountInfo() { return; }

    inline virtual ~IMountInfo() { return; }

    /**
     * Iterator for the list of mount points.
     */
    virtual std::vector<IMountPoint*> mountPoints() = 0;

};