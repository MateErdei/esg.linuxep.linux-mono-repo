/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <Common/FileSystem/IFileSystem.h>

#include <optional>
#include <regex>
#include <string>

namespace Common::UtilityImpl
{
    std::optional<std::string> extractValueFromIniFile(const Path& filePath, const std::string& key);
} // namespace Common::UtilityImpl
