/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <map>

namespace CentralRegistrationImpl
{
    void updateConfigOptions(std::string& key, std::string& value, std::map<std::string, std::string>& configOptions);
    std::map<std::string, std::string> processCommandLineOptions(int argc, char* argv[] );
    int main_entry(int argc, char* argv[]);

} // namespace CentralRegistrationImpl
