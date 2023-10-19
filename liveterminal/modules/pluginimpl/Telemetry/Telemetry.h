// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IFileSystem.h"

#include <optional>
#include <string>

namespace Plugin
{
    namespace Telemetry
    {
        std::optional<std::string> getVersion();
        void initialiseTelemetry();
    }
} // namespace Plugin