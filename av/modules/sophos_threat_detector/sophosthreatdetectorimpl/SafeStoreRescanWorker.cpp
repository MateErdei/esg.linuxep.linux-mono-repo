// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreRescanWorker.h"

#include "Logger.h"

#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"

#include "common/ApplicationPaths.h"

SafeStoreRescanWorker::SafeStoreRescanWorker(const fs::path& safeStoreRescanSocket) :
    m_safeStoreRescanSocket(safeStoreRescanSocket),
    m_rescanInterval(Plugin::getPluginVarDirPath(), "safeStoreRescanInterval", 14400)
{
    LOGDEBUG("SafeStore Rescan socket path " << safeStoreRescanSocket);
}

void SafeStoreRescanWorker::run()
{
    LOGDEBUG("Starting SafeStoreRescanWorker");

    announceThreadStarted();

    while (true)
    {
        std::unique_lock lock(m_rescanLock);
        m_rescanWakeUp.wait_for(
            lock,
            std::chrono::seconds(m_rescanInterval.getValue()),
            [this] { return m_manualRescan || m_notifyPipe.notified(); });

        if (m_notifyPipe.notified())
        {
            LOGDEBUG("Exiting SafeStoreRescanWorker");
            break;
        }

        unixsocket::SafeStoreRescanClient safeStoreRescanClient(m_safeStoreRescanSocket);
        safeStoreRescanClient.sendRescanRequest();
        m_manualRescan = false;
    }
}

void SafeStoreRescanWorker::triggerRescan()
{
    std::unique_lock lock(m_rescanLock);
    m_manualRescan = true;
    lock.unlock();
    m_rescanWakeUp.notify_one();
}

SafeStoreRescanWorker::~SafeStoreRescanWorker()
{
    m_notifyPipe.notify();
    m_rescanWakeUp.notify_one();
}
