/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <optional>
#include <string>
#include <chrono>

#include "MessageChannel.h"
#include <Common/DirectoryWatcher/IDirectoryWatcher.h>
#include <Common/DirectoryWatcher/IDirectoryWatcherListener.h>


namespace CommsComponent
{

    class MonitorDirClosedException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class MonitorDir{
        class MonitorDirListener : public Common::DirectoryWatcher::IDirectoryWatcherListener
        {
            MessageChannel m_channel; 
            std::string m_directoryPath; 
            std::string m_fileNameContainsFilter; 

            public: 
            MonitorDirListener(const std::string& directoryPath, const std::string& fileNameContainsFilter); 
            ~MonitorDirListener(); 

            std::string getPath() const override;

            void fileMoved(const std::string& filename) override;

            void watcherActive(bool active) override;
            MessageChannel& channel(); 
        };

        MonitorDirListener m_monitorDirListener; 
        std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
        const std::chrono::milliseconds NO_TIMEOUT{-1};

        public: 
        MonitorDir(const std::string & directoryPath, const std::string & fileNameContainsFilter); 
        ~MonitorDir(); 

        /* may throw MonitorDirClosedException after stop is issued*/
        std::optional<std::string> next(std::chrono::milliseconds); 
        std::optional<std::string> next();
        void stop();
    };
} // namespace CommsComponent

