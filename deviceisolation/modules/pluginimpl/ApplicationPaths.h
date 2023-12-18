// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>

namespace Plugin
{
    std::string getVersionIniFilePath();
    std::string pluginBinDir();
    std::string pluginTempDir();
    std::string pluginVarDir();
    std::string networkRulesFile();
    std::string nftBinary();
} // namespace Plugin
