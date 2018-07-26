/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IProcess.h"
#include <sys/types.h>
#include <functional>

namespace Common
{
    namespace ProcessImpl
    {
        class PipeHolder;
        class StdPipeThread;

        class ProcessImpl : public virtual Process::IProcess
        {

        public:
            ProcessImpl();
            ~ProcessImpl() override ;
            Process::ProcessStatus wait(Process::Milliseconds period, int attempts) override ;
            void exec( const std::string & path, const std::vector<std::string> & arguments,
                                const std::vector<Process::EnvironmentPair> & extraEnvironment) override;
            void exec( const std::string & path, const std::vector<std::string> & arguments) override;
            int exitCode() override;
            std::string output() override ;
            void kill() override ;
        private:
            pid_t m_pid;
            std::unique_ptr<PipeHolder> m_pipe;
            std::unique_ptr<StdPipeThread> m_pipeThread;
            int m_exitcode;
        };


        class ProcessFactory
        {
            ProcessFactory();
            std::function<std::unique_ptr<Common::Process::IProcess>(void)> m_creator;
        public:
            static ProcessFactory & instance();
            std::unique_ptr<Process::IProcess> createProcess();
            // for tests only
            void replaceCreator(std::function<std::unique_ptr<Common::Process::IProcess>(void)>creator);
            void restoreCreator();
        };
    }



}



