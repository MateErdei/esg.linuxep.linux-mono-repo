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

        static ActionType getType(const std::string actionJson);
    };

}

