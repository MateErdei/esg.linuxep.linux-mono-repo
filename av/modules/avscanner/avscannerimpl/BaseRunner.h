/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

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

        void setMountInfo(mountinfo::IMountInfoSharedPtr ptr) override;

    protected:
        BaseRunner();
        
        int m_returnCode = 0;
        std::shared_ptr<unixsocket::IScanningClientSocket> m_socket;
        mountinfo::IMountInfoSharedPtr m_mountInfo;
        std::shared_ptr<ScanCallbackImpl> m_scanCallbacks;

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

        bool walk(filewalker::FileWalker& filewalker, const sophos_filesystem::path& abspath,
                  const std::string& reportpath);
    };
}
