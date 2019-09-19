/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/IProcess.h>
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
            ~ProcessImpl() override;
            Process::ProcessStatus wait(Process::Milliseconds period, int attempts) override;
            int childPid() const override;
            void exec(
                const std::string& path,
                const std::vector<std::string>& arguments,
                const std::vector<Process::EnvironmentPair>& extraEnvironment,
                uid_t uid,
                gid_t gid) override;
            void exec(
                const std::string& path,
                const std::vector<std::string>& arguments,
                const std::vector<Process::EnvironmentPair>& extraEnvironment) override;
            void exec(const std::string& path, const std::vector<std::string>& arguments) override;
            int exitCode() override;
            std::string output() override;
            bool kill() override;
            bool kill( int secondsBeforeSIGKILL) override;

            Process::ProcessStatus getStatus() override;

            void setOutputLimit(size_t limit) override;
            void setOutputTrimmedCallback(std::function<void(std::string)>) override ;
            void setNotifyProcessFinishedCallBack(Process::IProcess::functor) override;

        private:
            void onExecFinished();
            pid_t m_pid;
            std::unique_ptr<PipeHolder> m_pipe;
            std::unique_ptr<StdPipeThread> m_pipeThread;
            int m_exitcode;
            size_t m_outputLimit;
            Process::IProcess::functor m_callback;
        };

        class ProcessFactory
        {
            ProcessFactory();
            std::function<std::unique_ptr<Common::Process::IProcess>(void)> m_creator;

        public:
            static ProcessFactory& instance();
            std::unique_ptr<Process::IProcess> createProcess();
            // for tests only
            void replaceCreator(std::function<std::unique_ptr<Common::Process::IProcess>(void)> creator);
            void restoreCreator();
        };
    } // namespace ProcessImpl

} // namespace Common
