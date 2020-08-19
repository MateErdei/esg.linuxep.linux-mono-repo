/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "avscanner/mountinfo/IMountInfo.h"

#include <unixsocket/threatDetectorSocket/IScanningClientSocket.h>

#include <memory>

namespace avscanner::avscannerimpl
{
    class IRunner
    {
    public:
        virtual ~IRunner() = default;
        virtual int run() = 0;
        virtual void setSocket(std::shared_ptr<unixsocket::IScanningClientSocket>) = 0;
        virtual void setMountInfo(mountinfo::IMountInfoSharedPtr) = 0;
    };
}
