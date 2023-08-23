// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/OSUtilities/ISystemUtils.h"
#include "cmcsrouter/ConfigOptions.h"

#include <map>
#include <memory>

namespace CentralRegistration
{
    MCS::ConfigOptions processCommandLineOptions(
        const std::vector<std::string>& args,
        const std::shared_ptr<OSUtilities::ISystemUtils>& systemUtils);
    MCS::ConfigOptions innerCentralRegistration(
        const std::vector<std::string>& args,
        const std::string& mcsCertPath = "");
    int main_entry(int argc, char* argv[]);

} // namespace CentralRegistration
