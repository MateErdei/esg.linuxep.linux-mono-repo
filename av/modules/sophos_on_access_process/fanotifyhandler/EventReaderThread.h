//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "IFanotifyHandler.h"

#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/Exclusion.h"
#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/Threads/NotifyPipe.h"

#include <sys/fanotify.h>

namespace fs = sophos_filesystem;
namespace onaccessimpl = sophos_on_access_process::onaccessimpl;

namespace sophos_on_access_process::fanotifyhandler
{
    class EventReaderThread : public common::AbstractThreadPluginInterface
    {
    public:
        EventReaderThread(
            IFanotifyHandlerSharedPtr fanotify,
            datatypes::ISystemCallWrapperSharedPtr sysCalls,
            const fs::path& pluginInstall,
            onaccessimpl::ScanRequestQueueSharedPtr scanRequestQueue);

        void run() override;

        void setExclusions(std::vector<common::Exclusion> exclusions);

    private:
        bool handleFanotifyEvent();
        bool skipScanningOfEvent(struct fanotify_event_metadata* eventMetadata, std::string& filePath, std::string& exePath);
        std::string getFilePathFromFd(int fd);
        std::string getUidFromPid(pid_t pid);

        IFanotifyHandlerSharedPtr m_fanotify;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fs::path m_pluginLogDir;
        onaccessimpl::ScanRequestQueueSharedPtr m_scanRequestQueue;
        pid_t m_pid;
        std::string m_processExclusionStem;
        std::vector<common::Exclusion> m_exclusions;
    };
}
