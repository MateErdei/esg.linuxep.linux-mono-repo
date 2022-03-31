/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITask.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>

#include <stdexcept>
#include <string>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        class ActionTask : public virtual Common::TaskQueue::ITask
        {
        public:
            ActionTask(PluginCommunication::IPluginManager& pluginManager, const std::string& filePath);
            void run() override;
            static bool isAlive(const std::string& ttl);

        private:
            PluginCommunication::IPluginManager& m_pluginManager;
            std::string m_filePath;
        };


        class FailedToConvertTtlException : public std::runtime_error
        {
        public:
            virtual ~FailedToConvertTtlException() = default;
            explicit FailedToConvertTtlException(const std::string& what) : std::runtime_error(what) {}
        };
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
