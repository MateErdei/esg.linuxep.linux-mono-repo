/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DiagnoseResultDirectoryListener.h"

namespace RemoteDiagnoseImpl::runnerModule
{
    DiagnoseResultDirectoryListener::DiagnoseResultDirectoryListener(
        const std::string& directory,
        const std::string& nameOfFileToWaitFor) :
        m_directoryToWatch(directory),
        m_foundFilePath(""),
        m_nameOfFileToWaitFor(nameOfFileToWaitFor),
        m_active(false),
        m_fileFound(false),
        m_aborted(false)
    {
    }

    std::string DiagnoseResultDirectoryListener::getPath() const { return m_directoryToWatch; }

    void DiagnoseResultDirectoryListener::fileMoved(const std::string& filename)
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

    void DiagnoseResultDirectoryListener::watcherActive(bool active) { m_active = active; }

    std::string DiagnoseResultDirectoryListener::waitForFile(std::chrono::seconds timeoutInSeconds)
    {
        std::unique_lock<std::mutex> lock(m_filenameMutex);
        if (shouldStop())
        {
            return m_foundFilePath;
        }
        m_fileDetected.wait_for(lock, timeoutInSeconds, [this]() { return shouldStop(); });
        return m_foundFilePath;
    }

    void DiagnoseResultDirectoryListener::abort()
    {
        m_aborted = true;
        m_fileDetected.notify_all();
    }

    bool DiagnoseResultDirectoryListener::wasAborted() { return m_aborted; }

    bool DiagnoseResultDirectoryListener::shouldStop() { return m_fileFound || m_aborted; }
} // namespace RemoteDiagnoseImpl::runnerModule
