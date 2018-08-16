/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderResultDirectoryListener.h"

SulDownloaderResultDirectoryListener::SulDownloaderResultDirectoryListener(const std::string& directory, const std::string& nameOfFileToWaitFor)
        : m_foundFilePath(""), m_directoryToWatch(directory), m_active(false), m_fileFound(false), m_aborted(false), m_nameOfFileToWaitFor(nameOfFileToWaitFor)
{
}

std::string SulDownloaderResultDirectoryListener::getPath() const
{
    return m_directoryToWatch;
}

void SulDownloaderResultDirectoryListener::fileMoved(const std::string & filename)
{
    if (filename != m_nameOfFileToWaitFor)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_filenameMutex);
    m_foundFilePath = filename;
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
        return m_foundFilePath;
    }
    m_fileDetected.wait_for(lock, timeoutInSeconds,[this]() { return shouldStop(); });
    return m_foundFilePath;
}

void SulDownloaderResultDirectoryListener::abort()
{
    m_aborted = true;
    m_fileDetected.notify_all();
}

bool SulDownloaderResultDirectoryListener::wasAborted()
{
    return m_aborted;
}

bool SulDownloaderResultDirectoryListener::shouldStop()
{
    return m_fileFound || m_aborted;
}
