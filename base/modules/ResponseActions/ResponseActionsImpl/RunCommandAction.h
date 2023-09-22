// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ActionsUtils.h"

#include "Common/ProcessMonitoring/ISignalHandler.h"
#include "Common/SystemCallWrapper/ISystemCallWrapperFactory.h"

#include <string>

namespace ResponseActionsImpl
{
    inline void to_json(nlohmann::json& j, const SingleCommandResult& singleCommandResult)
    {
        j = nlohmann::json{ { "duration", singleCommandResult.duration },
                            { "exitCode", singleCommandResult.exitCode },
                            { "stdErr", singleCommandResult.stdErr },
                            { "stdOut", singleCommandResult.stdOut } };
    }
    inline void to_json(nlohmann::json& j, const CommandResponse& cmdResponse)
    {
        j = nlohmann::json{ { "type", cmdResponse.type },
                            { "result", cmdResponse.result },
                            { "commandResults", cmdResponse.commandResults },
                            { "duration", cmdResponse.duration },
                            { "startedAt", cmdResponse.startedAt } };
    }

    class RunCommandAction
    {
    public:
        RunCommandAction(
            Common::ISignalHandlerSharedPtr sigHandler,
            Common::SystemCallWrapper::ISystemCallWrapperFactorySharedPtr sysCallFactory);
        RunCommandAction() = delete;

        nlohmann::json run(const std::string& actionJson, const std::string& correlationId);
        CommandResponse runCommands(const CommandRequest& action);
        SingleCommandResult runCommand(const std::string& command);

    private:
        Common::ISignalHandlerSharedPtr m_SignalHandler;
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr m_SysCallWrapper;

        bool m_terminate = false;
        bool m_timeout = false;
    };
} // namespace ResponseActionsImpl