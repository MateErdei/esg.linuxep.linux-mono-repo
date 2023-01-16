// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "SafeStoreRescanWorker.h"

#include "Logger.h"

#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"

#include "common/ApplicationPaths.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"


namespace {

    class rescanStoppableSleeper : public common::StoppableSleeper
    {
    public:
        explicit rescanStoppableSleeper(std::unique_lock<std::mutex>& lock, bool& stopRequested, std::condition_variable& rescanWakeup)
        : m_lock(lock),
        m_stopRequested(stopRequested),
        m_rescanWakeup(rescanWakeup)
        { }

        bool stoppableSleep(common::StoppableSleeper::duration_t sleepTime) override
        {
            return m_rescanWakeup.wait_for(m_lock, sleepTime, [this]() { return m_stopRequested; });
        }

    private:
        std::unique_lock<std::mutex>& m_lock;
        bool& m_stopRequested;
        std::condition_variable& m_rescanWakeup;
    };
}

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
    {
        std::unique_lock lock(m_rescanLock);
        m_stopRequested = true;
    }
    m_rescanWakeUp.notify_all();
}

void SafeStoreRescanWorker::run()
{
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
        sendRescanRequest(lock);
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

void SafeStoreRescanWorker::sendRescanRequest(std::unique_lock<std::mutex>& lock)
{
    auto sleeper = std::make_shared<rescanStoppableSleeper>(lock, m_stopRequested, m_rescanWakeUp);
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



