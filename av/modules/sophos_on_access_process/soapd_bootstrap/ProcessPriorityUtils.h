// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/ISystemCallWrapper.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    void setThreatDetectorPriority(int level, const datatypes::ISystemCallWrapperSharedPtr& sysCalls);
}
