// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ThreatDetectorResources.h"

#include "datatypes/SystemCallWrapperFactory.h"

using namespace sspl::sophosthreatdetectorimpl;


datatypes::ISystemCallWrapperSharedPtr ThreatDetectorResources::createSystemCallWrapper()
{
    auto sysCallFact = datatypes::SystemCallWrapperFactory();
    return sysCallFact.createSystemCallWrapper();
}