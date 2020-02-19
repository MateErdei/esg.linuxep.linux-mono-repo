/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"

#include <unixsocket/IScanningClientSocket.h>

#include <memory>

namespace avscanner::avscannerimpl
{
    class BaseRunner : public IRunner
    {
    public:
        void setSocket(std::shared_ptr<unixsocket::IScanningClientSocket> ptr) override;

    protected:
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
    };
}
