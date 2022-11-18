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
        std::unique_lock lock(m_QMCheckLock);
        while (!m_stopRequested)
        {
            m_checkWakeUp.wait_for(lock,m_reinitialiseBackoff, [this]{ return m_stopRequested.load(); });
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
        increaseBackOff();
        auto state = m_quarantineManager->getState();
        switch (state)
        {
            case QuarantineManagerState::STARTUP:
                LOGDEBUG("In startup");

                try
                {
                    m_quarantineManager->initialise();
                    m_reinitialiseBackoff = 60s;
                }
                catch (const std::exception& ex)
                {
                    LOGERROR("Failed to initialise SafeStore database: " << ex.what());
                }

                break;
            case QuarantineManagerState::INITIALISED:
                m_reinitialiseBackoff = 60s;
                LOGDEBUG("Quarantine Manager is initialised");
                break;
            case QuarantineManagerState::UNINITIALISED:
                LOGWARN("Quarantine Manager is not initialised");

                try
                {
                    m_quarantineManager->initialise();
                    m_reinitialiseBackoff = 60s;
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
                        m_reinitialiseBackoff = 60s;
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

    void StateMonitor::increaseBackOff()
    {
        m_reinitialiseBackoff *= 2;
        if (m_reinitialiseBackoff > m_maxReinitialiseBackoff)
        {
            m_reinitialiseBackoff = m_maxReinitialiseBackoff;
        }
    }

} // namespace safestore::QuarantineManager