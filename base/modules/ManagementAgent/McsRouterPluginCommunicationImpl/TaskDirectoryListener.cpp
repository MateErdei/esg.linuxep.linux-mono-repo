/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TaskDirectoryListener.h"

#include "ActionTask.h"
#include "PolicyTask.h"

#include <Common/FileSystem/IFileSystem.h>
#include <ManagementAgent/LoggerImpl/Logger.h>

#include <cassert>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        TaskDirectoryListener::TaskDirectoryListener(
            const std::string& directoryPath,
            ITaskQueueSharedPtr taskQueue,
            PluginCommunication::IPluginManager& pluginManager) :
            m_pluginManager(pluginManager),
            m_directoryPath(directoryPath),
            m_taskQueue(taskQueue),
            m_active(false)
        {
        }

        std::string TaskDirectoryListener::getPath() const { return m_directoryPath; }

        void TaskDirectoryListener::fileMoved(const std::string& filename)
        {
            assert(Common::FileSystem::basename(filename) == filename);

            Common::TaskQueue::ITaskPtr task;

            LOGDEBUG("filename=" << filename);

            if (filename.find("policy") != std::string::npos || filename.find("flags") != std::string::npos)
            {
                task.reset(new PolicyTask(m_pluginManager, filename));
            }
            else if (filename.find("action") != std::string::npos || filename.find("request") != std::string::npos)
            {
                task.reset(new ActionTask(m_pluginManager, filename));
            }
            else
            {
                LOGWARN("Invalid file " << filename << " moved into " << getPath());
                return;
            }

            assert(task != nullptr);
            m_taskQueue->queueTask(std::move(task));
        }

        void TaskDirectoryListener::watcherActive(bool active) { m_active = active; }
    } // namespace McsRouterPluginCommunicationImpl
} // namespace ManagementAgent
