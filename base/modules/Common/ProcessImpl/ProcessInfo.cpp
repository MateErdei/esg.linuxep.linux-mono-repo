// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ProcessInfo.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFilePermissions.h"

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

namespace Common
{
    namespace ProcessImpl
    {
        ProcessInfo::ProcessInfo() noexcept : m_secondsToShutDown(2), m_executableUser(-1), m_executableGroup(-1) {}

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

        void ProcessInfo::refreshUserAndGroupIds()
        {
            setExecutableUserAndGroup(m_executableUserAndGroupAsString);
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

            auto ifperms = Common::FileSystem::filePermissions();

            try
            {
                std::pair<uid_t, gid_t> userAndGroupId =
                    ifperms->getUserAndGroupId(userName.c_str());

                m_executableUser = userAndGroupId.first;
                if (groupName.empty())
                {
                    m_executableGroup = userAndGroupId.second;
                }
                else
                {
                    m_executableGroup = ifperms->getGroupId(groupName);
                }
            }
            catch (const Common::FileSystem::IFileSystemException& exception)
            {
                LOGERROR(exception.what());
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

        int ProcessInfo::getSecondsToShutDown() const { return m_secondsToShutDown; }

        void ProcessInfo::setSecondsToShutDown(int seconds) { m_secondsToShutDown = seconds; }

    } // namespace ProcessImpl
} // namespace Common
namespace Common::Process
{
    IProcessInfoPtr createEmptyProcessInfo() { return IProcessInfoPtr(new Common::ProcessImpl::ProcessInfo); }
} // namespace Common::Process