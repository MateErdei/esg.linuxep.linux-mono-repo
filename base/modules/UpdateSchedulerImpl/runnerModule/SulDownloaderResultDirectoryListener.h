/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/DirectoryWatcher/IDirectoryWatcher.h>

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        class SulDownloaderResultDirectoryListener
                : public Common::DirectoryWatcher::IDirectoryWatcherListener
        {

        public:
            explicit SulDownloaderResultDirectoryListener(const std::string& directory,
                                                          const std::string& nameOfFileToWaitFor);

            std::string getPath() const override;

            void fileMoved(const std::string& filename) override;

            void watcherActive(bool active) override;

            /// waitForFile will block until m_stop is true, which indicates either the file has been detected or that the
            /// abort method has been called and this update should be aborted.
            /// \param timeoutInSeconds
            /// \return the path to the results file or "" if no file created in time or abort called.
            std::string waitForFile(std::chrono::seconds timeout);

            void abort();

            bool wasAborted();

            bool shouldStop();

        private:
            std::string m_directoryToWatch;
            std::string m_foundFilePath;
            std::string m_nameOfFileToWaitFor;
            bool m_active;
            bool m_fileFound;
            bool m_aborted;
            std::mutex m_filenameMutex;
            std::condition_variable m_fileDetected;

        };

    }
}

