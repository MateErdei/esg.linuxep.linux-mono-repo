// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionsUtils.h"

#include <string>

namespace ResponseActionsImpl
{
    void to_json(nlohmann::json& j, const SingleCommandResult& singleCommandResult);
    void to_json(nlohmann::json& j, const CommandResponse& cmdResponse);

    namespace RunCommandAction
    {
        nlohmann::json run(const std::string& actionJson, const std::string& correlationId);
        CommandRequest parseCommandAction(const std::string& actionJson);
        CommandResponse runCommands(const CommandRequest& action, const std::string& correlationId);
        SingleCommandResult runCommand(const std::string& command);
    } // namespace RunCommandAction
} // namespace ResponseActionsImpl