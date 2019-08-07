/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessInfo.h"

#include "Logger.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

namespace Common
{
    namespace ProcessImpl
    {
        ProcessInfo::ProcessInfo() noexcept : m_executableUser(-1), m_executableGroup(-1) {}

        std::vector<std::string> ProcessInfo::getExecutableArguments() const { return m_executableArguments; }

        Process::EnvPairs ProcessInfo::getExecutableEnvironmentVariables() const
        {
            return m_executableEnvironmentVariables;
        }

        std::string ProcessInfo::getExecutableFullPath() const { return m_executableFullPath; }

        void ProcessInfo::setExecutableFullPath(const std::string& executableFullPath)
        {
            m_executableFullPath = executableFullPath;
        }

        void ProcessInfo::setExecutableArguments(const std::vector<std::string>& executableArguments)
        {
            m_executableArguments = executableArguments;
        }

        void ProcessInfo::addExecutableArguments(const std::string& executableArgument)
        {
            m_executableArguments.push_back(executableArgument);
        }

        void ProcessInfo::setExecutableEnvironmentVariables(const Process::EnvPairs& executableEnvironmentVariables)
        {
            m_executableEnvironmentVariables = executableEnvironmentVariables;
        }

        void ProcessInfo::addExecutableEnvironmentVariables(
            const std::string& environmentName,
            const std::string& environmentValue)
        {
            m_executableEnvironmentVariables.emplace_back(environmentName, environmentValue);
        }

        void ProcessInfo::setExecutableUserAndGroup(const std::string& executableUserAndGroup)
        {
            m_executableUser = -1;
            m_executableGroup = -1;

            m_executableUserAndGroupAsString = executableUserAndGroup;

            size_t pos = executableUserAndGroup.find(':');

            std::string userName = executableUserAndGroup.substr(0, pos);

            std::string groupName;
            if (pos != std::string::npos)
            {
                groupName = executableUserAndGroup.substr(pos + 1);
            }

            struct passwd* passwdStruct;
            passwdStruct = ::getpwnam(userName.c_str());

            if (passwdStruct != nullptr)
            {
                m_executableUser = passwdStruct->pw_uid;

                if (groupName.empty())
                {
                    m_executableGroup = passwdStruct->pw_gid;
                }
                else
                {
                    FileSystem::FilePermissionsImpl filefunctions;
                    try
                    {
                        int groupId = filefunctions.getGroupId(groupName);
                        m_executableGroup = groupId;
                    }
                    catch (const Common::FileSystem::IFileSystemException& exception)
                    {
                        LOGERROR(exception.what());
                    }
                }
            }
        }

        std::string ProcessInfo::getExecutableUserAndGroupAsString() const { return m_executableUserAndGroupAsString; }

        std::pair<bool, uid_t> ProcessInfo::getExecutableUser() const
        {
            uid_t userId = static_cast<uid_t>(m_executableUser);

            return std::make_pair(m_executableUser != -1, userId);
        }

        std::pair<bool, gid_t> ProcessInfo::getExecutableGroup() const
        {
            gid_t groupId = static_cast<gid_t>(m_executableGroup);

            return std::make_pair(m_executableGroup != -1, groupId);
        }

    } // namespace ProcessImpl
} // namespace Common
namespace Common::Process
{
    IProcessInfoPtr createEmptyProcessInfo() { return IProcessInfoPtr(new Common::ProcessImpl::ProcessInfo); }
} // namespace Common::Process