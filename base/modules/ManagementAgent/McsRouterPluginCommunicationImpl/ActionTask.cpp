/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ActionTask.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <ManagementAgent/LoggerImpl/Logger.h>

namespace
{
    struct ActionFilenameFields
    {
        std::string m_appId;
        std::string m_correlationId;
        bool m_isValid;
        bool m_isAlive;
    };

    ActionFilenameFields getActionFilenameFields(const std::string& filename)
    {
        ActionFilenameFields actionFilenameFields;
        actionFilenameFields.m_isValid = false;
        actionFilenameFields.m_isAlive = true;

        auto fileNameFields = Common::UtilityImpl::StringUtils::splitString(filename, "_");
        if (Common::UtilityImpl::StringUtils::isSubstring(filename, "LiveQuery_") && fileNameFields.size() == 5)
        {
            actionFilenameFields.m_appId = fileNameFields[0];
            actionFilenameFields.m_correlationId = fileNameFields[1];
            actionFilenameFields.m_isAlive = ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive(fileNameFields[3]);
            actionFilenameFields.m_isValid = true;
        }
        else if (Common::UtilityImpl::StringUtils::isSubstring(filename, "LiveTerminal_") && fileNameFields.size() == 4)
        {
            // TODO: LINUXDAR-1648  Consolidate this 'else if' with the above 'if' when correlationId is added for LiveTerminal.
            actionFilenameFields.m_appId = fileNameFields[0];
            actionFilenameFields.m_isAlive = ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive(fileNameFields[3]);
            actionFilenameFields.m_isValid = true;
        }
        else
        {
            if (Common::UtilityImpl::StringUtils::isSubstring(filename, "_action_") && Common::UtilityImpl::StringUtils::endswith(filename, ".xml") )
            {
                actionFilenameFields.m_appId = fileNameFields[0];
                actionFilenameFields.m_correlationId = "";
                actionFilenameFields.m_isValid = true;
            }
        }
        return actionFilenameFields;
    }
}
ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::ActionTask(
    ManagementAgent::PluginCommunication::IPluginManager& pluginManager,
    const std::string& filePath) :
    m_pluginManager(pluginManager),
    m_filePath(filePath)
{
}

bool ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::isAlive(const std::string& ttl)
{
    std::time_t nowTime = Common::UtilityImpl::TimeUtils::getCurrTime();
    std::time_t long_ttl = 0;
    try
    {
        long_ttl = std::stol(ttl);
    }
    catch (std::exception& exception)
    {
        std::stringstream msg;
        msg << "Failed to convert time to live '" << ttl << "' into time_t";
        throw FailedToConvertTtlException(msg.str());
    }
    return long_ttl >= nowTime;
}

void ManagementAgent::McsRouterPluginCommunicationImpl::ActionTask::run()
{
    LOGSUPPORT("Process new action from mcsrouter: " << m_filePath);
    std::string basename = Common::FileSystem::basename(m_filePath);
    ActionFilenameFields actionFilenameFields;
    try
    {
        actionFilenameFields = getActionFilenameFields(basename);
    }
    catch (FailedToConvertTtlException& exception)
    {
        LOGERROR(exception.what());
        return;
    }
    if (!actionFilenameFields.m_isValid)
    {
        LOGWARN("Got an invalid file name for action detection: " << m_filePath);
        return;
    }

    if (!actionFilenameFields.m_isAlive)
    {
        LOGWARN("Action has expired: " << m_filePath);
        return;
    }

    std::string payload;
    try
    {
        payload = Common::FileSystem::fileSystem()->readFile(m_filePath);
    }
    catch (Common::FileSystem::IFileSystemException& e)
    {
        LOGERROR("Failed to read " << m_filePath << " with error: " << e.what());
        throw;
    }

    int pluginsNotified = m_pluginManager.queueAction(actionFilenameFields.m_appId, payload, actionFilenameFields.m_correlationId);
    LOGINFO("Action " << m_filePath << " sent to " << pluginsNotified << " plugins");

    Common::FileSystem::fileSystem()->removeFile(m_filePath);
}
