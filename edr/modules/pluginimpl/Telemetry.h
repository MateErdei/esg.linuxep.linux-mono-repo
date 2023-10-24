// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IFileSystem.h"

#include <optional>
#include <string>

namespace plugin
{
    std::optional<std::string> getVersion();
    std::optional<unsigned long> getOsqueryDatabaseSize();
    void processOsqueryLogLineForEventsMaxTelemetry(std::string& line);
    void readOsqueryInfoFiles();
} // namespace Plugin