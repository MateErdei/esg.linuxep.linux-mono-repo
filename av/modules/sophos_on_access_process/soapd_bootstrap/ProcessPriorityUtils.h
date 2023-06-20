// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/SystemCallWrapper/ISystemCallWrapper.h"

namespace sophos_on_access_process::soapd_bootstrap
{
    void setThreatDetectorPriority(int level, const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr& sysCalls);
}
