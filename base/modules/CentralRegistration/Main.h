/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/OSUtilities/ISystemUtils.h>
#include <cmcsrouter/ConfigOptions.h>

#include <map>
#include <memory>

namespace CentralRegistrationImpl
{
    //void updateConfigOptions(std::string& key, std::string& value, std::map<std::string, std::string>& configOptions);
    MCS::ConfigOptions processCommandLineOptions(const std::vector<std::string>& args, std::shared_ptr<OSUtilities::ISystemUtils> systemUtils);
    int main_entry(int argc, char* argv[]);
    MCS::ConfigOptions innerCentralRegistration(const std::vector<std::string>& args, std::shared_ptr<OSUtilities::ISystemUtils> systemUtils);

} // namespace CentralRegistrationImpl
