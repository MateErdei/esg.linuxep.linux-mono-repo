/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IProcessHolder.h"

#include <Common/Process/IProcess.h>
#include <sys/types.h>

#include <atomic>
#include <functional>
#include <mutex>

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
            int nativeExitCode() override;
            std::string output() override;
            std::string errorOutput() override;
            std::string standardOutput() override;
            bool kill() override;
            bool kill(int secondsBeforeSIGKILL) override;
            void sendSIGUSR1() override;

            Process::ProcessStatus getStatus() override;

            void setOutputLimit(size_t limit) override;
            void setFlushBufferOnNewLine(bool flushOnNewLine) override;
            void setOutputTrimmedCallback(std::function<void(std::string)>) override;
            void setNotifyProcessFinishedCallBack(Process::IProcess::functor) override;

            void waitUntilProcessEnds() override;
            void setCoreDumpMode(const bool mode) override;

        private:
            std::mutex m_protectImplOnBoost;
            std::atomic<int> m_pid;
            size_t m_outputLimit;
            bool m_flushOnNewLine;
            Process::IProcess::functor m_callback;
            std::function<void(std::string)> m_notifyTrimmed;
            bool m_coreDumpEnabled = false;
            // in order to protect for data race, this pointer will
            // need to be shared pointer to be used via this safeAccess method.
            std::shared_ptr<IProcessHolder> safeAccess();
            std::shared_ptr<IProcessHolder> m_d;
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
