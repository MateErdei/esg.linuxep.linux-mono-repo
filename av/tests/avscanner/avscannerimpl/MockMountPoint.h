/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <gmock/gmock.h>

#include "avscanner/avscannerimpl/IMountPoint.h"

class MockMountPoint : public avscanner::avscannerimpl::IMountPoint
{
public:
    MOCK_CONST_METHOD0(device, std::string());
    MOCK_CONST_METHOD0(filesystemType, std::string());
    MOCK_CONST_METHOD0(isHardDisc, bool());
    MOCK_CONST_METHOD0(isNetwork, bool());
    MOCK_CONST_METHOD0(isOptical, bool());
    MOCK_CONST_METHOD0(isRemovable, bool());
    MOCK_CONST_METHOD0(isSpecial, bool());
    MOCK_CONST_METHOD0(mountPoint, std::string());
};