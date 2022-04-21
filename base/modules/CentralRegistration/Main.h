/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/ISystemUtils.h>
#include <cmcsrouter/ConfigOptions.h>

#include <map>
#include <memory>

namespace CentralRegistration
{
    MCS::ConfigOptions processCommandLineOptions(const std::vector<std::string>& args, std::shared_ptr<OSUtilities::ISystemUtils> systemUtils);
    MCS::ConfigOptions innerCentralRegistration(const std::vector<std::string>& args);
    int main_entry(int argc, char* argv[]);

} // namespace CentralRegistrationImpl
