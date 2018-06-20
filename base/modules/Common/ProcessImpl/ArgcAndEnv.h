///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_ARGCANDENV_H
#define EVEREST_BASE_ARGCANDENV_H
#include <vector>
#include <string>
#include "IProcess.h"
#include "IProcessException.h"

namespace Common
{
    namespace ProcessImpl
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
                ArgcAdaptor()
                {};

                void setArgs(std::vector<std::string> arguments, std::string path = std::string())
                {
                    m_arguments.clear();
                    if (!path.empty())
                    {
                        m_arguments.push_back(path);
                    }
                    for (auto &arg: arguments)
                    {
                        m_arguments.push_back(arg);
                    }

                    // const_cast is needed because execvp prototype wants an
                    // array of char*, not const char*.
                    for (auto const &arg : m_arguments)
                    {
                        m_argcAdaptor.emplace_back(const_cast<char *>(arg.c_str()));
                    }
                    // NULL terminate
                    m_argcAdaptor.push_back(nullptr);

                }

                char **argcpointer()
                {
                    if (m_argcAdaptor.empty())
                        return nullptr;
                    return m_argcAdaptor.data();
                }

            private:
                std::vector<std::string> m_arguments;
                std::vector<char *> m_argcAdaptor;
            };

            ArgcAdaptor m_argc;
            ArgcAdaptor m_env;
            std::string m_path;
        public:
            ArgcAndEnv(std::string path, std::vector<std::string> arguments,
                       std::vector<Common::Process::EnvironmentPair> extraEnvironment)
            {
                m_path = path;
                std::vector<std::string> envArguments;
                for (auto &env : extraEnvironment)
                {
                    // make sure all environment variables are valid.
                    if (env.first.empty())
                    {
                        throw Common::Process::IProcessException("Environment name cannot be empty: '' = " + env.second
                        );
                    }
                    envArguments.push_back(env.first + '=' + env.second);
                }
                m_argc.setArgs(arguments, path);
                m_env.setArgs(envArguments);
            }

            char **argc()
            {
                return m_argc.argcpointer();
            }

            char **envp()
            {
                return m_env.argcpointer();
            }

            char *path()
            {
                return const_cast<char *>( m_path.data());

            }
        };

    }
}
#endif //EVEREST_BASE_ARGCANDENV_H
