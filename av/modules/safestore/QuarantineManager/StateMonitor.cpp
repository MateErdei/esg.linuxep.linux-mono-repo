// Copyright 2022, Sophos Limited.  All rights reserved.

#include "StateMonitor.h"

#include "safestore/Logger.h"

#include <utility>

namespace safestore::QuarantineManager
{
    StateMonitor::StateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager) :
        m_quarantineManager(std::move(quarantineManager))
    {
    }

    StateMonitor::~StateMonitor()
    {
        tryStop();
        join();
    }

    void StateMonitor::tryStop()
    {
        {
            std::lock_guard lock(m_QMCheckLock);
            m_stopRequested = true;
        }
        m_checkWakeUp.notify_one();
    }
    void StateMonitor::run()
    {
        announceThreadStarted();
        LOGINFO("Starting Quarantine Manager state monitor");
        while (!m_stopRequested)
        {
//            auto now = std::chrono::system_clock::now().time_since_epoch();
//            auto nextTimeToCheckState =
//                std::min(m_lastCheck + m_reinitialiseBackoff, m_lastCheck + m_maxReinitialiseBackoff);
//            if (nextTimeToCheckState < now)
//            {
//                innerRun();
//            }
//            else
//            {
//                std::this_thread::sleep_for(std::chrono::milliseconds(100));
//            }

            std::unique_lock lock(m_QMCheckLock);

            if (m_reinitialiseBackoff >= m_maxReinitialiseBackoff)
            {
                m_reinitialiseBackoff = 60s;
            }

            m_checkWakeUp.wait_for(lock,m_reinitialiseBackoff);
            if (m_stopRequested)
            {
                LOGDEBUG("State Monitor stop requested");
                break;
            }
            innerRun();
        }
    }

    void StateMonitor::innerRun()
    {
        m_reinitialiseBackoff = m_reinitialiseBackoff * 2;
//        m_lastCheck = std::chrono::system_clock::now().time_since_epoch();
        auto state = m_quarantineManager->getState();
        switch (state)
        {
            case QuarantineManagerState::STARTUP:
                LOGDEBUG("In startup");

                try
                {
                    m_quarantineManager->initialise();
                }
                catch (const std::exception& ex)
                {
                    LOGERROR("Failed to initialise SafeStore database: " << ex.what());
                }

                break;
            case QuarantineManagerState::INITIALISED:
                LOGDEBUG("Quarantine Manager is initialised");
                break;
            case QuarantineManagerState::UNINITIALISED:
                LOGWARN("Quarantine Manager is not initialised");

                try
                {
                    m_quarantineManager->initialise();
                }
                catch (const std::exception& ex)
                {
                    LOGERROR("Failed to initialise SafeStore database: " << ex.what());
                }

                break;
            case QuarantineManagerState::CORRUPT:
                LOGWARN("Quarantine Manager is corrupt");
                try
                {
                    bool removedSafeStoreDb = m_quarantineManager->deleteDatabase();

                    if (removedSafeStoreDb)
                    {
                        LOGDEBUG("Successfully removed corrupt SafeStore database");
                        m_quarantineManager->initialise();
                        break;
                    }

                    LOGERROR("Failed to remove corrupt SafeStore database");
                }
                catch (const std::exception& ex)
                {
                    LOGERROR("Failed to clean up corrupt SafeStore database: " << ex.what());
                }
                break;
        }
    }

} // namespace safestore::QuarantineManager