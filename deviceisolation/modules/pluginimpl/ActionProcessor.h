// Copyright 2023 Sophos All rights reserved.
#pragma once

#include <optional>
#include <string>

namespace Plugin
{
    class ActionProcessor
    {
    public:
        /**
         * Parse the Isolate action.
         * @param xml Action XML
         * @return nullopt for invalid actions, and true/false for valid actions
         */
        static std::optional<bool> processIsolateAction(const std::string& xml);
    };
}