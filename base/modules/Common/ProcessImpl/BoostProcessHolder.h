// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IProcessHolder.h"

#include "Common/Threads/LockableData.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"
#include <boost/asio/io_service.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/process/child.hpp>

#pragma GCC diagnostic pop

#include <future>

namespace Common
{
    namespace ProcessImpl
    {
        struct ProcessResult
        {
            int exitCode;
            std::string output;
            std::string errorlog;
            std::string combinedoutput;
            int nativeExitCode = 0;
        };

        struct BoostChildProcessDestructor
        {
            void operator()(boost::process::child* p)
            {
                p->wait();
                delete p;
            }
        };

        class BoostProcessHolder : public IProcessHolder
        {
        public:
            BoostProcessHolder(
                const std::string& path,
                const std::vector<std::string>& arguments,
                const std::vector<Process::EnvironmentPair>& extraEnvironment,
                uid_t uid,
                gid_t gid,
                Process::IProcess::functor callback,
                std::function<void(std::string)> notifyTrimmed,
                size_t outputLimit,
                bool flushOnNewLine);
            ~BoostProcessHolder();
            int pid() override;

            void wait() override;

            Process::ProcessStatus wait(std::chrono::milliseconds timeToWait) override;

            int exitCode() override;
            int nativeExitCode() override;

            std::string output() override;
            std::string stderroutput() override;
            std::string stdoutput() override;

            bool hasFinished() override;

            void sendTerminateSignal() override;

            void sendAbortSignal() override;
            void kill() override;

        private:
            void armAsyncReaderForChildStdOutput();
            void armAsyncReaderForChildStdErr();
            void handleOutMessage(const boost::system::error_code& ec, std::size_t size);
            void handleErrMessage(const boost::system::error_code& ec, std::size_t size);
            std::size_t completionCondition(const boost::system::error_code& ec, std::size_t size,std::vector<char>& buffer);
            bool shouldBufferBeFlushed(std::size_t size,std::vector<char>& buffer);
            ProcessResult waitChildProcessToFinish();
            std::future<ProcessResult> asyncWaitChildProcessToFinish();
            void cacheResult();
            void cacheResultLocked(std::unique_lock<std::timed_mutex>& locked);
            std::timed_mutex m_onCacheResult;
            std::mutex m_outputAccess;
            Process::IProcess::functor m_callback;
            std::function<void(std::string)> m_notifyTrimmed;
            std::shared_future<ProcessResult> m_result;

            std::string m_path;
            std::vector<char> m_bufferForIOServiceErr;
            std::vector<char> m_bufferForIOServiceOut;
            std::string m_stdout;
            std::string m_stderr;
            boost::asio::io_service asioIOService;
            boost::process::async_pipe asyncPipeErr;
            boost::process::async_pipe asyncPipeOut;
            std::unique_ptr<boost::process::child, BoostChildProcessDestructor> m_child;
            int m_pid;

            /* handle the results */
            std::atomic<Process::ProcessStatus> m_status;
            size_t m_outputLimit;
            std::atomic<bool> m_finished;

            ProcessResult m_cached;
            bool m_enableBufferFlushOnNewLine;

            Common::Threads::LockableData<bool> deliberatelyKilled_;
            bool deliberatelyKilled();
            void deliberatelyKilled(bool);
        };
    } // namespace ProcessImpl
} // namespace Common
