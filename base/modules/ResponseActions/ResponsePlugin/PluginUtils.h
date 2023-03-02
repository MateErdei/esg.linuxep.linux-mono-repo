// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "ActionType.h"

#include <json.hpp>
#include <string>
namespace ResponsePlugin
{

    class PluginUtils
    {
    public:
        PluginUtils() = default;
        ~PluginUtils() = default;
        static std::pair<std::string, int> getType(const std::string& actionJson);
    };

}

