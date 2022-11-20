// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreRescanWorker.h"

#include "Logger.h"

#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"

#include "common/ApplicationPaths.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"

SafeStoreRescanWorker::SafeStoreRescanWorker(const fs::path& safeStoreRescanSocket) :
    m_safeStoreRescanSocket(safeStoreRescanSocket), m_rescanInterval(parseRescanIntervalConfig())
{
    LOGDEBUG("SafeStore Rescan socket path: " << safeStoreRescanSocket);
    LOGDEBUG("SafeStore Rescan interval: " << m_rescanInterval);
}

SafeStoreRescanWorker::~SafeStoreRescanWorker()
{
    tryStop();
    join();
}

void SafeStoreRescanWorker::tryStop()
{
    m_stopRequested = true;
    m_rescanWakeUp.notify_all();
}

void SafeStoreRescanWorker::run()
{
    LOGDEBUG("Starting SafeStoreRescanWorker");

    announceThreadStarted();

    std::unique_lock lock(m_rescanLock);
    while (!m_stopRequested)
    {
        m_rescanWakeUp.wait_for(
            lock, std::chrono::seconds(m_rescanInterval), [this] { return m_manualRescan || m_stopRequested; });

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
    auto sleeper = std::make_shared<rescanStoppableSleeper>(*this);
    unixsocket::SafeStoreRescanClient safeStoreRescanClient(
        m_safeStoreRescanSocket, std::chrono::seconds { 1 }, sleeper);
    safeStoreRescanClient.sendRescanRequest();
}

uint SafeStoreRescanWorker::parseRescanIntervalConfig()
{
    auto* fs = Common::FileSystem::fileSystem();

    if (fs->exists(Plugin::getRelativeSafeStoreRescanIntervalConfigPath()))
    {
        LOGDEBUG("SafeStore Rescan Worker interval setting file found -- attempting to parse.");
        try
        {
            auto contents = fs->readFile(Plugin::getRelativeSafeStoreRescanIntervalConfigPath());
            auto [value, errorMessage] = Common::UtilityImpl::StringUtils::stringToInt(contents);
            if (!errorMessage.empty())
            {
                LOGDEBUG(
                    "Error parsing integer for rescan interval setting: " << contents << " due to: " << errorMessage);
            }
            else if (value <= 0)
            {
                LOGDEBUG("Rescan interval cannot be less than 1. Value found: " << contents);
            }
            else
            {
                LOGINFO("Setting rescan interval to: " << contents << " seconds");
                return value;
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGDEBUG("Could not read rescanInterval file due to: " << e.what());
        }
        LOGINFO("Setting rescan interval to default 4 hours");
    }
    return 14400;
}

SafeStoreRescanWorker::rescanStoppableSleeper::rescanStoppableSleeper(SafeStoreRescanWorker& parent) : m_parent(parent)
{
}

bool SafeStoreRescanWorker::rescanStoppableSleeper::stoppableSleep(common::StoppableSleeper::duration_t sleepTime)
{
    std::unique_lock<std::mutex> lck(m_parent.m_sleeperLock);
    return m_parent.m_rescanWakeUp.wait_for(lck, sleepTime, [this]() { return m_parent.m_stopRequested.load(); });
}
