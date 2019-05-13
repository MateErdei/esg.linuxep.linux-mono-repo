/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Process/IProcessInfo.h>

namespace Common
{
    namespace ProcessImpl
    {
        class ProcessInfo
                : public Common::Process::IProcessInfo
        {
        public:
            ProcessInfo() noexcept;
            /**
            * Used to get the path of the plugin executable.
            * @return string containing the path
            */
            std::string getExecutableFullPath() const override;

            /**
             * Used to get the required arguments for the plugin executable (if any)
             * @return zero or more strings used for the executable arguments.
             */
            std::vector<std::string> getExecutableArguments() const override;

            /**
             * Used to get the required environment variables for the plugin executable (if any)
             * @return zero or more strings used for the executable environment variables.
             */
            Process::EnvPairs getExecutableEnvironmentVariables() const override;

            /**
             * Used to store the full part to the plugin executable
             * @param executableFullPath
             */
            void setExecutableFullPath(const std::string& executableFullPath) override;

            /**
             * Used to store the given arguments the plugin requires to run.
             * @param list of executableArguments
             */
            void setExecutableArguments(const std::vector<std::string>& executableArguments) override;

            /**
             * Used to add the given argument to the stored list of arguments.
             * @param executableArgument
             */
            void addExecutableArguments(const std::string& executableArgument) override;

            /**
             * Used to store a list of environment variables required by the plugin.
             * @param executableEnvironmentVariables
             */
            void
            setExecutableEnvironmentVariables(const Common::Process::EnvPairs& executableEnvironmentVariables) override;

            /**
             * Used to add a single environment variable to the list of stored environment variables required by the
             * plugin.
             * @param environmentName
             * @param environmentValue
             */
            void addExecutableEnvironmentVariables(
                    const std::string& environmentName,
                    const std::string& environmentValue) override;

            /**
             * Set the User an Group to execute the child process with, specified user and group must exist, or -1 will
             * be set.
             * @param executableUserAndGroup string in the form "user:group" or "user"
             */
            void setExecutableUserAndGroup(const std::string& executableUserAndGroup) override;

            /**
             *
             * @return Executable user and group in the form "user:group" or "user"
             */
            std::string getExecutableUserAndGroupAsString() const override;

            /**
             * gets user id relating to the Executable user
             * @return pair <true, valid user id> if the user id is valid, pair <false, invalid user id> otherwise
             */
            std::pair<bool, uid_t> getExecutableUser() const override;

            /**
             * gets group id relating to the Executable group
             * @return pair <true, valid group id> if the group id is valid, pair <false, invalid group id> otherwise
             */
            std::pair<bool, gid_t> getExecutableGroup() const override;

        protected:
            int m_executableUser;
            int m_executableGroup;
            std::string m_executableFullPath;
            std::vector<std::string> m_executableArguments;
            Process::EnvPairs m_executableEnvironmentVariables;
            std::string m_executableUserAndGroupAsString;
        };

    }
}



