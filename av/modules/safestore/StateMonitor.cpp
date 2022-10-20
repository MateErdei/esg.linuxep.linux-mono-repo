// Copyright 2022, Sophos Limited.  All rights reserved.

#include "StateMonitor.h"

#include "Logger.h"

#include <utility>

namespace safestore
{
    StateMonitor::StateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager) :
        m_quarantineManager(std::move(quarantineManager))
    {
    }

    void StateMonitor::run()
    {
        announceThreadStarted();
        LOGINFO("Starting Quarantine Manager state monitor");
        while (!stopRequested())
        {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto nextTimeToCheckState =
                std::min(m_lastCheck + m_reinitialiseBackoff, m_lastCheck + m_maxReinitialiseBackoff);
            if (nextTimeToCheckState < now)
            {
                innerRun();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void StateMonitor::innerRun()
    {
        m_reinitialiseBackoff = m_reinitialiseBackoff * 2;
        m_lastCheck = std::chrono::system_clock::now().time_since_epoch();
        auto state = m_quarantineManager->getState();
        switch (state)
        {
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

} // namespace safestore