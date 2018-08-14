/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderResultDirectoryListener.h"

#pragma once

SulDownloaderResultDirectoryListener::SulDownloaderResultDirectoryListener(const std::string &path)
        : m_Path(path), m_Active(false)
{
}

std::string SulDownloaderResultDirectoryListener::getPath() const
{
    return m_Path;
}

void SulDownloaderResultDirectoryListener::fileMoved(const std::string & filename)
{
    std::unique_lock<std::mutex> lock(m_FilenameMutex);
    m_File = filename;
    m_HasData = true;
    m_fileDetected.notify_one();
}

void SulDownloaderResultDirectoryListener::watcherActive(bool active)
{
    m_Active = active;
}

std::string SulDownloaderResultDirectoryListener::waitForFile(unsigned timeoutInSeconds)
{
    std::chrono::seconds duration(timeoutInSeconds);
    std::unique_lock<std::mutex> lock(m_FilenameMutex);
    m_fileDetected.wait_for(lock, duration);
    return m_HasData ? m_File : std::string();
}
