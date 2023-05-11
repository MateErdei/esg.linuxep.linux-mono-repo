// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "IFanotifyHandler.h"

#include "ExecutablePathCache.h"

#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"
#include "sophos_on_access_process/onaccessimpl/IOnAccessTelemetryUtility.h"

#include "common/AbstractThreadPluginInterface.h"
#include "common/Exclusion.h"
#include "common/UsernameSetting.h"
#include "datatypes/ISystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"
#include "mount_monitor/mountinfo/IDeviceUtil.h"

#include "Common/Threads/NotifyPipe.h"

#include <mutex>
#include <sys/fanotify.h>

namespace fs = sophos_filesystem;
namespace onaccessimpl = sophos_on_access_process::onaccessimpl;

namespace sophos_on_access_process::fanotifyhandler
{
    constexpr int LOG_NOT_FULL_THRESHOLD = 100;

    class EventReaderThread : public common::AbstractThreadPluginInterface
    {
    public:
        using scan_request_t = ::onaccessimpl::ScanRequestQueue::scan_request_t;

        EventReaderThread(
            IFanotifyHandlerSharedPtr fanotify,
            datatypes::ISystemCallWrapperSharedPtr sysCalls,
            const fs::path& pluginInstall,
            onaccessimpl::ScanRequestQueueSharedPtr scanRequestQueue,
            onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility,
            mount_monitor::mountinfo::IDeviceUtilSharedPtr deviceUtil,
            int logNotFullThreshold = LOG_NOT_FULL_THRESHOLD);

        void run() override;

        void setExclusions(const std::vector<common::Exclusion>& exclusions);
        void setDetectPUAs(bool detectPUAs);

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
#ifdef USERNAME_UID_USED
        std::string getUidFromPid(pid_t pid);
#endif
        void throwIfErrorNotRecoverable();

        bool cacheIfAllowed(const scan_request_t& request);

        ExecutablePathCache executablePathCache_;
        IFanotifyHandlerSharedPtr m_fanotify;
        datatypes::ISystemCallWrapperSharedPtr m_sysCalls;
        fs::path m_pluginLogDir;
        onaccessimpl::ScanRequestQueueSharedPtr m_scanRequestQueue;
        onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr m_telemetryUtility;
        mount_monitor::mountinfo::IDeviceUtilSharedPtr m_deviceUtil;
        pid_t m_pid;
        std::string m_processExclusionStem;
        std::vector<common::Exclusion> m_exclusions;
        bool m_detectPUAs = true;
        mutable std::mutex m_exclusionsLock;
        uint m_EventsWhileQueueFull = 0;
        int m_readFailureCount = 0;
        bool m_cacheAllEvents = false;
        const int m_logNotFullThreshold;
    };
}
