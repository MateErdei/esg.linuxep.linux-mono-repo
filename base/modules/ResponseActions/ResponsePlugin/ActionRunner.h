// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once
#include "IActionRunner.h"

#include <Common/Process/IProcess.h>

#include <future>
namespace ResponsePlugin
{
    class ActionRunner : public IActionRunner
    {
    public:
        void runAction(
            const std::string& action,
            const std::string& correlationId,
            const std::string& type,
            int timeout) override;
        void killAction() override;
        bool getIsRunning() override;

    private:
        bool isRunning = false;
        Common::Process::IProcessPtr m_process;
        std::string m_executablePath;
        std::future<void> m_fut;
    };
} // namespace ResponsePlugin