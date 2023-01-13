// Copyright 2023, Sophos Limited.  All rights reserved.

#include "Logger.h"
#include "ThreatDetectorControlCallback.h"

using namespace sspl::sophosthreatdetectorimpl;

ThreatDetectorControlCallback::ThreatDetectorControlCallback(ISophosThreatDetectorMainPtr mainInstance)
    : m_mainInstance(std::move(mainInstance))
{}

void ThreatDetectorControlCallback::processControlMessage(const scan_messages::E_COMMAND_TYPE& command)
{
    switch (command)
    {
        case scan_messages::E_SHUTDOWN:
            m_mainInstance->shutdownThreatDetector();
            break;
        case scan_messages::E_RELOAD:
            m_mainInstance->reloadSUSIGlobalConfiguration();
            break;
        default:
            LOGWARN("Sophos On Access Process received unknown process control message");
    }
}
