/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SulDownloaderRunner.h"
//#include "../../tests/Common/DirectoryWatcherImpl/DummyDirectoryWatcherListener.h"


namespace UpdateScheduler
{

    SulDownloaderRunner::SulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, std::string directoryToWatch)
    {

        m_directoryWatcher = std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher>(new Common::DirectoryWatcherImpl::DirectoryWatcher());

        //DirectoryWatcherListener directoryWatcherListener(directoryToWatch);


        m_directoryWatcher->addListener(directoryWatcherListener);
        m_directoryWatcher->startWatch();

    }

    int SulDownloaderRunner::run()
    {
        // call systemd start SULDownloader
        // wait with a timeout on download and install
        // once done save item onto queue sayign it is complete.
        return 0;
    }


}