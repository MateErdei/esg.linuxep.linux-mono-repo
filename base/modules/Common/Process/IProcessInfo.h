/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/EnvPair.h>

#include <memory>
#include <string>
#include <vector>

namespace Common
{
    namespace Process
    {
        using EnvPairs = Common::Process::EnvPairVector;

        class IProcessInfo
        {
        public:
            virtual ~IProcessInfo() = default;
            /**
             * Used to get the path of the plugin executable.
             * @return string containing the path
             */
            virtual std::string getExecutableFullPath() const = 0;

            /**
             * Used to get the required arguments for the plugin executable (if any)
             * @return zero or more strings used for the executable arguments.
             */
            virtual std::vector<std::string> getExecutableArguments() const = 0;

            /**
             * Used to get the required environment variables for the plugin executable (if any)
             * @return zero or more strings used for the executable environment variables.
             */
            virtual EnvPairs getExecutableEnvironmentVariables() const = 0;

            /**
             * Used to store the full part to the plugin executable
             * @param executableFullPath
             */
            virtual void setExecutableFullPath(const std::string& executableFullPath) = 0;

            /**
             * Used to store the given arguments the plugin requires to run.
             * @param list of executableArguments
             */
            virtual void setExecutableArguments(const std::vector<std::string>& executableArguments) = 0;

            /**
             * Used to add the given argument to the stored list of arguments.
             * @param executableArgument
             */
            virtual void addExecutableArguments(const std::string& executableArgument) = 0;

            /**
             * Used to store a list of environment variables required by the plugin.
             * @param executableEnvironmentVariables
             */
            virtual void setExecutableEnvironmentVariables(
                const Common::Process::EnvPairs& executableEnvironmentVariables) = 0;

            /**
             * Used to add a single environment variable to the list of stored environment variables required by the
             * plugin.
             * @param environmentName
             * @param environmentValue
             */
            virtual void addExecutableEnvironmentVariables(
                const std::string& environmentName,
                const std::string& environmentValue) = 0;

            /**
             * Set the User an Group to execute the child process with, specified user and group must exist, or -1 will
             * be set.
             * @param executableUserAndGroup string in the form "user:group" or "user"
             */
            virtual void setExecutableUserAndGroup(const std::string& executableUserAndGroup) = 0;

            /**
             *
             * @return Executable user and group in the form "user:group" or "user"
             */
            virtual std::string getExecutableUserAndGroupAsString() const = 0;

            /**
             * gets user id relating to the Executable user
             * @return pair <true, valid user id> if the user id is valid, pair <false, invalid user id> otherwise
             */
            virtual std::pair<bool, uid_t> getExecutableUser() const = 0;

            /**
             * gets group id relating to the Executable group
             * @return pair <true, valid group id> if the group id is valid, pair <false, invalid group id> otherwise
             */
            virtual std::pair<bool, gid_t> getExecutableGroup() const = 0;
        };

        using IProcessInfoPtr = std::unique_ptr<IProcessInfo>;
        extern IProcessInfoPtr createEmptyProcessInfo();
    } // namespace Process
} // namespace Common