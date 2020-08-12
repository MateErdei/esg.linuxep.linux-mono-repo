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

        void setMountInfo(mountinfo::IMountInfoSharedPtr ptr) override;

    protected:
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
        mountinfo::IMountInfoSharedPtr m_mountInfo;

        /**
         * Get or create mount info.
         * @return
         */
        mountinfo::IMountInfoSharedPtr getMountInfo();
        /**
         * Get or create a scanning socket
         * @return
         */
        std::shared_ptr<unixsocket::IScanningClientSocket> getSocket();
    };
}
