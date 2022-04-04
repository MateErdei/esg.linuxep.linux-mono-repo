/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <cmcsrouter/ConfigOptions.h>

#include <map>

namespace CentralRegistrationImpl
{
    //void updateConfigOptions(std::string& key, std::string& value, std::map<std::string, std::string>& configOptions);
    MCS::ConfigOptions processCommandLineOptions(int argc, char* argv[] );
    int main_entry(int argc, char* argv[]);

} // namespace CentralRegistrationImpl
