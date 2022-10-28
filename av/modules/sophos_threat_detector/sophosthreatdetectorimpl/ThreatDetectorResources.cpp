// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ThreatDetectorResources.h"

#include "common/signals/SigTermMonitor.h"
#include "common/PidLockFile.h"
#include "datatypes/SystemCallWrapperFactory.h"

using namespace sspl::sophosthreatdetectorimpl;


datatypes::ISystemCallWrapperSharedPtr ThreatDetectorResources::createSystemCallWrapper()
{
    auto sysCallFact = datatypes::SystemCallWrapperFactory();
    return sysCallFact.createSystemCallWrapper();
}

common::signals::ISignalHandlerSharedPtr ThreatDetectorResources::createSignalHandler(bool restartSyscalls)
{
    return std::make_shared<common::signals::SigTermMonitor>(restartSyscalls);
}

common::IPidLockFileSharedPtr ThreatDetectorResources::createPidLockFile(const std::string& _path)
{
    return std::make_shared<common::PidLockFile>(_path);
}