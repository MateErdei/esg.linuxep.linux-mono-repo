// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include <string>

namespace Plugin
{
    std::string getPluginInstall();
    std::string getChrootDir();
    std::string getSafeStoreFlagPathWithinChroot();
    std::string getSafeStoreFlagPath();
} // namespace Plugin
