// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "ActionType.h"

#include <string>
namespace ResponsePlugin
{

    class PluginUtils
    {
    public:
        PluginUtils() = default;
        ~PluginUtils() = default;

        /*
         * Returns the action type and the timeout in seconds
         */
        static std::pair<std::string, int> getType(const std::string& actionJson);
    };

} // namespace ResponsePlugin
