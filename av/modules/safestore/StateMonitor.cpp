// Copyright 2022, Sophos Limited.  All rights reserved.

#include "StateMonitor.h"

#include "Logger.h"

#include <utility>

namespace safestore
{
    void StateMonitor::run()
    {
        announceThreadStarted();
        LOGINFO("Starting Quarantine Manager state monitor");
        // nextTimeToCheckState
        while (!stopRequested())
        {
            auto state = m_quarantineManager->getState();
            // TODO check state of QM, if not in good state try to fix it etc...
            // When done verifying this set this time to 100ms and then make sure there is no logging done unless state
            // changes.
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            switch (state)
            {
                case QuarantineManagerState::INITIALISED:
                    LOGINFO("Quarantine Manager is INITIALISED");
                    break;
                case QuarantineManagerState::UNINITIALISED:
                    LOGINFO("Quarantine Manager is UNINITIALISED");
                    break;
                case QuarantineManagerState::CORRUPT:
                    LOGINFO("Quarantine Manager is CORRUPT");
                    break;
            }
        }
    }
    StateMonitor::StateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager) :
        m_quarantineManager(std::move(quarantineManager))
    {
    }
} // namespace safestore