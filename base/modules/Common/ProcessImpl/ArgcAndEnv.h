// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessException.h"

#include <cassert>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

namespace Common::ProcessImpl
{
    /**
     * execve expects argc and envp to be given as char**.
     * This class allows the expected pointers to be passed to the execve without needing to handle
     * allocations and deallocations internally. It uses vector to handle all the allocations correctly.
     * It is safe do to so as execve will anyway put the arguments and enviroment variables in a 'safe place'
     * before changing the image of the process.
     */

    class ArgcAndEnv
    {
        class ArgcAdaptor
        {
        public:
            ArgcAdaptor() = default;

            void setArgs(const std::vector<std::string>& arguments, const std::string& path = std::string())
            {
                m_arguments.clear();
                if (!path.empty())
                {
                    m_arguments.push_back(path);
                }
                for (auto& arg : arguments)
                {
                    m_arguments.push_back(arg);
                }

                // const_cast is needed because execvp prototype wants an
                // array of char*, not const char*.
                for (auto const& arg : m_arguments)
                {
                    m_argcAdaptor.emplace_back(const_cast<char*>(arg.c_str()));
                }
                // NULL terminate
                m_argcAdaptor.push_back(nullptr);
            }

            char** argcpointer() { return m_argcAdaptor.data(); }

        private:
            std::vector<std::string> m_arguments;
            std::vector<char*> m_argcAdaptor;
        };

        ArgcAdaptor m_argc;
        ArgcAdaptor m_env;
        std::string m_path;

    public:
        ArgcAndEnv(
            const std::string& path,
            const std::vector<std::string>& arguments,
            const std::vector<Common::Process::EnvironmentPair>& extraEnvironment)
        {
            m_path = path;
            m_argc.setArgs(arguments, path);
            if (!extraEnvironment.empty())
            {
                m_env.setArgs(mergeWithEnvironmentVariables(extraEnvironment));
            }

            std::vector<std::string> envArguments;
            for (auto& env : extraEnvironment)
            {
                // make sure all environment variables are valid.
                if (env.first.empty())
                {
                    throw Common::Process::IProcessException("Environment name cannot be empty: '' = " + env.second);
                }
                envArguments.push_back(env.first + '=' + env.second);
            }
        }

        char** argc() { return m_argc.argcpointer(); }

        char** envp() { return m_env.argcpointer(); }

        char* path() { return const_cast<char*>(m_path.data()); }

    private:
        std::string getEnvironmentNameFromEnvP(const std::string& envpPair) const
        {
            size_t pos = envpPair.find('=');

            assert(pos != std::string::npos);

            return envpPair.substr(0, pos);
        }

        std::vector<std::string> mergeWithEnvironmentVariables(
            const std::vector<Common::Process::EnvironmentPair>& extraEnvironment)
        {
            std::vector<std::string> envArguments;
            std::set<std::string> uniqueEnvironmentNames;

            for (auto& env : extraEnvironment)
            {
                // make sure all environment variables are valid.
                if (env.first.empty())
                {
                    throw Common::Process::IProcessException("Environment name cannot be empty: '' = " + env.second);
                }
                envArguments.push_back(env.first + '=' + env.second);
                uniqueEnvironmentNames.insert(env.first);
            }

            for (char** env = environ; *env != nullptr; env++)
            {
                char* current = *env;
                std::string envName = getEnvironmentNameFromEnvP(current);
                if (uniqueEnvironmentNames.find(envName) == uniqueEnvironmentNames.end())
                {
                    envArguments.emplace_back(current);
                }
            }
            return envArguments;
        }
    };
} // namespace Common::ProcessImpl
