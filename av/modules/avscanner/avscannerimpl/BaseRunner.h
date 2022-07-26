/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"
#include "ScanCallbackImpl.h"

#include <filewalker/FileWalker.h>
#include <unixsocket/threatDetectorSocket/IScanningClientSocket.h>
#include "ScanCallbackImpl.h"

#include <memory>

namespace avscanner::avscannerimpl
{
    class BaseRunner : public IRunner
    {
    public:
        void setSocket(std::shared_ptr<unixsocket::IScanningClientSocket> ptr) override;

        void setMountInfo(mount_monitor::mountinfo::IMountInfoSharedPtr ptr) override;

    protected:
        BaseRunner();
        
        int m_returnCode = common::E_CLEAN_SUCCESS;
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
        mount_monitor::mountinfo::IMountInfoSharedPtr m_mountInfo;
        std::shared_ptr<ScanCallbackImpl> m_scanCallbacks;

        /**
         * Get or create mount info.
         * @return
         */
        mount_monitor::mountinfo::IMountInfoSharedPtr getMountInfo();
        /**
         * Get or create a scanning socket
         * @return
         */
        std::shared_ptr<unixsocket::IScanningClientSocket> getSocket();

        bool walk(filewalker::FileWalker& filewalker, const sophos_filesystem::path& abspath,
                  const std::string& reportpath);
    };
}
