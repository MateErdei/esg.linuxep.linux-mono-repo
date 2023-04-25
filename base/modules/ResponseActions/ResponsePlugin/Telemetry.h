// Copyright 2023 Sophos Limited. All rights reserved.


#pragma once

#include <map>
#include <optional>
#include <string>

namespace ResponsePlugin
{
    std::optional<std::string> getVersion();

    void incrementTotalActions(const std::string& type);
    void incrementFailedActions(const std::string& type);
    void incrementTimedOutActions(const std::string& type);
    void incrementExpiredActions(const std::string& type);

} // namespace ResponsePlugin