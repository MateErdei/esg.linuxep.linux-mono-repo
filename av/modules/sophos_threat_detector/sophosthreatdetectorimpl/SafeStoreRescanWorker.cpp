// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreRescanWorker.h"

#include "Logger.h"

#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"

#include "common/ApplicationPaths.h"

SafeStoreRescanWorker::SafeStoreRescanWorker(const fs::path& safeStoreRescanSocket) :
    m_safeStoreRescanSocket(safeStoreRescanSocket),
    m_rescanInterval(Plugin::getPluginChrootVarDirPath(), "safeStoreRescanInterval", 14400)
{
    LOGDEBUG("SafeStore Rescan socket path: " << safeStoreRescanSocket);
    LOGDEBUG("SafeStore Rescan interval: " << m_rescanInterval.getValue());
}

SafeStoreRescanWorker::~SafeStoreRescanWorker()
{
    {
        std::lock_guard lock(m_rescanLock);
        m_stopRequested = true;
    }
    m_rescanWakeUp.notify_one();
    join();
}

void SafeStoreRescanWorker::run()
{
    LOGDEBUG("Starting SafeStoreRescanWorker");

    announceThreadStarted();

    while (!m_stopRequested)
    {
        std::unique_lock lock(m_rescanLock);
        m_rescanWakeUp.wait_for(
            lock,
            std::chrono::seconds(m_rescanInterval.getValue()),
            [this] { return m_manualRescan || m_stopRequested; });

        if (m_stopRequested)
        {
            LOGDEBUG("SafeStoreRescanWorker stop requested");
            break;
        }

        // else if ( m_manualRescan || wait_for(timeout))
        sendRescanRequest();
        m_manualRescan = false;
    }
    LOGDEBUG("Exiting SafeStoreRescanWorker");
}

void SafeStoreRescanWorker::triggerRescan()
{
    {
        std::lock_guard lock(m_rescanLock);
        m_manualRescan = true;
    }
    m_rescanWakeUp.notify_one();
}

void SafeStoreRescanWorker::sendRescanRequest()
{
    unixsocket::SafeStoreRescanClient safeStoreRescanClient(m_safeStoreRescanSocket);
    safeStoreRescanClient.sendRescanRequest();
}
