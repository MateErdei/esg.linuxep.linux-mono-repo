// Copyright 2022 Sophos Limited. All rights reserved.
//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "IFanotifyHandler.h"

#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"
#include "sophos_on_access_process/onaccessimpl/OnAccessTelemetryUtility.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/Exclusion.h"
#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/Threads/NotifyPipe.h"

#include <mutex>
#include <sys/fanotify.h>

namespace fs = sophos_filesystem;
namespace onaccessimpl = sophos_on_access_process::onaccessimpl;

namespace sophos_on_access_process::fanotifyhandler
{
    class EventReaderThread : public common::AbstractThreadPluginInterface
    {
    public:
        using scan_request_t = ::onaccessimpl::ScanRequestQueue::scan_request_t;

        EventReaderThread(
            IFanotifyHandlerSharedPtr fanotify,
            datatypes::ISystemCallWrapperSharedPtr sysCalls,
            const fs::path& pluginInstall,
            onaccessimpl::ScanRequestQueueSharedPtr scanRequestQueue,
            onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr telemetryUtility
            );

        void run() override;

        void setExclusions(const std::vector<common::Exclusion>& exclusions);

        void setCacheAllEvents(bool enable);

    TEST_PUBLIC:
        void innerRun();
        std::chrono::milliseconds m_out_of_file_descriptor_delay = std::chrono::milliseconds{100};
        static constexpr int RESTART_SOAP_ERROR_COUNT = 20;

    private:
        bool handleFanotifyEvent();
        bool skipScanningOfEvent(
            struct fanotify_event_metadata* eventMetadata, std::string& filePath, std::string& exePath, int eventFd);
        std::string getFilePathFromFd(int fd);
        std::string getUidFromPid(pid_t pid);
        void throwIfErrorNotRecoverable();

        IFanotifyHandlerSharedPtr m_fanotify;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fs::path m_pluginLogDir;
        onaccessimpl::ScanRequestQueueSharedPtr m_scanRequestQueue;
        onaccessimpl::onaccesstelemetry::OnAccessTelemetryUtilitySharedPtr m_telemetryUtility;
        pid_t m_pid;
        std::string m_processExclusionStem;
        std::vector<common::Exclusion> m_exclusions;
        mutable std::mutex m_exclusionsLock;
        uint m_EventsWhileQueueFull = 0;
        int m_readFailureCount = 0;
        bool m_cacheAllEvents = false;
    };
}
