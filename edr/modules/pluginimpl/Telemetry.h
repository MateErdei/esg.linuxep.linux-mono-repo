/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/FileSystem/IFileSystem.h>

#include <optional>
#include <string>

namespace plugin
{
    std::optional<std::string> getVersion();
} // namespace Plugin