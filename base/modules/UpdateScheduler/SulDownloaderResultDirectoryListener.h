/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/DirectoryWatcher/IDirectoryWatcher.h>

class SulDownloaderResultDirectoryListener : public Common::DirectoryWatcher::IDirectoryWatcherListener
{

public:
    explicit SulDownloaderResultDirectoryListener(const std::string &path);

    std::string getPath() const override;
    void fileMoved(const std::string & filename) override;
    void watcherActive(bool active) override;
    std::string  waitForFile(std::chrono::seconds timeout);

private:
    std::string m_Path;
    std::string m_File;
    bool m_Active;
    bool m_HasData;
    std::mutex m_FilenameMutex;
    std::condition_variable m_fileDetected;
};
