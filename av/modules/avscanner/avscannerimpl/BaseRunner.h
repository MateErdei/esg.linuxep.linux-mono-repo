/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"

#include <unixsocket/threatDetectorSocket/IScanningClientSocket.h>

#include <memory>

namespace avscanner::avscannerimpl
{
    class BaseRunner : public IRunner
    {
    public:
        void setSocket(std::shared_ptr<unixsocket::IScanningClientSocket> ptr) override;

        void setMountInfo(std::shared_ptr<IMountInfo> ptr) override;

    protected:
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
        std::shared_ptr<IMountInfo> m_mountInfo;

        /**
         * Get or create mount info.
         * @return
         */
        std::shared_ptr<IMountInfo> getMountInfo();
        /**
         * Get or create a scanning socket
         * @return
         */
        std::shared_ptr<unixsocket::IScanningClientSocket> getSocket();
    };
}
