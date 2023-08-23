// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/TaskQueue/ITask.h"
#include "ManagementAgent/PluginCommunication/IPluginManager.h"

#include <string>

namespace ManagementAgent::McsRouterPluginCommunicationImpl
{
    class PolicyTask : public virtual Common::TaskQueue::ITask
    {
    public:
        void run() override;
        PolicyTask(
            PluginCommunication::IPluginManager& pluginManager,
            std::string filePath,
            const std::string& pluginName = "");

    private:
        PluginCommunication::IPluginManager& m_pluginManager;
        std::string m_filePath;
        std::string m_pluginName;
    };
} // namespace ManagementAgent::McsRouterPluginCommunicationImpl
