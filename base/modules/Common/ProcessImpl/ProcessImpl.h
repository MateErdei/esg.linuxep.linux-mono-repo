/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_COMMON_PROCESS_IPROCESSIMPL_H
#define EVEREST_BASE_COMMON_PROCESS_IPROCESSIMPL_H
#include "IProcess.h"
#include <sys/types.h>
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
            ~ProcessImpl();
            Process::ProcessStatus wait(Process::Milliseconds period, int attempts) override ;
            void exec( const std::string & path, const std::vector<std::string> & arguments,
                                const std::vector<Process::EnvironmentPair> & extraEnvironment={}) override;
            int exitCode() override;
            std::string output() override ;
            void kill() override ;
        private:
            pid_t m_pid;
            std::unique_ptr<PipeHolder> m_pipe;
            std::unique_ptr<StdPipeThread> m_pipeThread;
            int m_exitcode;
        };
    }
}


#endif //EVEREST_BASE_COMMON_PROCESS_IPROCESSIMPL_H
