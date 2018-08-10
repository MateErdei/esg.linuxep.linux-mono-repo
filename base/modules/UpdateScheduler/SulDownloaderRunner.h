/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SchedulerTaskQueue.h"
#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>

namespace UpdateScheduler
{
    class SulDownloaderRunner
    {
    public:
        SulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, std::string directoryToWatch);

        //SulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, Common::DirectoryWatcher::IDirectoryWatcher directoryWatcher);
        int run();

    private:
        //std::unique_ptr<oryListener> m_policyListener;
        //std::unique_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::TaskDirectoryListener> m_actionListener;
        std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
    };

}