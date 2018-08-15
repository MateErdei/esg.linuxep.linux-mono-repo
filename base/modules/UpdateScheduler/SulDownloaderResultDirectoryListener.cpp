/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderResultDirectoryListener.h"

SulDownloaderResultDirectoryListener::SulDownloaderResultDirectoryListener(const std::string &path)
        : m_file(""), m_path(path), m_active(false), m_fileFound(false), m_aborted(false)
{
}

std::string SulDownloaderResultDirectoryListener::getPath() const
{
    return m_path;
}

void SulDownloaderResultDirectoryListener::fileMoved(const std::string & filename)
{
    if (filename != "report.json")
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_filenameMutex);
    m_file = filename;
    m_fileFound = true;
    m_fileDetected.notify_all();
}

void SulDownloaderResultDirectoryListener::watcherActive(bool active)
{
    m_active = active;
}

std::string SulDownloaderResultDirectoryListener::waitForFile(std::chrono::seconds timeoutInSeconds)
{
    std::unique_lock<std::mutex> lock(m_filenameMutex);
    if (shouldStop())
    {
        return m_file;
    }
    m_fileDetected.wait_for(lock, timeoutInSeconds,[this](){return shouldStop();});
    return m_file;
}

void SulDownloaderResultDirectoryListener::abort()
{
    m_aborted = true;
}

bool SulDownloaderResultDirectoryListener::wasAborted()
{
    return m_aborted;
}

bool SulDownloaderResultDirectoryListener::shouldStop()
{
    return m_fileFound || m_aborted;
}
