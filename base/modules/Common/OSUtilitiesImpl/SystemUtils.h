/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "Common/OSUtilities/ISystemUtils.h"

namespace OSUtilitiesImpl
{
    class SystemUtils : public OSUtilities::ISystemUtils
    {
    public:
         std::string getEnvironmentVariable(const std::string& key) const override;
    };
}
