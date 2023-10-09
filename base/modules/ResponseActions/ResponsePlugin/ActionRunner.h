// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once
#include "IActionRunner.h"

#include "ResponseActions/RACommon/ResponseActionsCommon.h"

#include <Common/Process/IProcess.h>

#include <future>
namespace ResponsePlugin
{
    /*
     * In the case where the Response Actions plugin has to forcibly terminate its Action Runner child process
     * we will not get a response written out by Action Runner, so we send a failure response from the RA plugin.
     */
    void sendFailedResponse(ResponseActions::RACommon::ResponseResult result, const std::string& type, const std::string& correlationId);

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