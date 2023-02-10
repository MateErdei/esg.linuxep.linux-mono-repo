// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "CentralEnums.h"
#include <json.hpp>

#include <string>
namespace ResponsePlugin
{

    class PluginUtils
    {
    public:
        PluginUtils() = default;
        ~PluginUtils() = default;
        static ActionType getType(const std::string& actionJson);
        static void sendResponse(const std::string& correlationId,const std::string& content);
    };

}

