/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IMountInfo.h"

#include <unixsocket/IScanningClientSocket.h>

#include <memory>

namespace avscanner::avscannerimpl
{
    class IRunner
    {
    public:
        virtual int run() = 0;
        virtual void setSocket(std::shared_ptr<unixsocket::IScanningClientSocket>) = 0;
        virtual void setMountInfo(std::shared_ptr<IMountInfo>) = 0;
    };
}
