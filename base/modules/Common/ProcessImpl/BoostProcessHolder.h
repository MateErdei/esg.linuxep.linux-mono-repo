/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IProcessHolder.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"
#include <boost/process/child.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/asio/io_service.hpp>

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
        };

        struct BoostChildProcessDestructor
        {
            void operator()(boost::process::child * p)
            {
                p->wait();
                delete p;
            }
        };

        class BoostProcessHolder: public  IProcessHolder
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
                    size_t outputLimit);
            ~BoostProcessHolder();
             int pid() override;

             void wait() override;

             Process::ProcessStatus wait(std::chrono::milliseconds timeToWait) override;

             int exitCode() override;

             std::string output() override;

             bool hasFinished() override;

             void sendTerminateSignal() override;

             void kill() override;

        private:
            void armAsyncReaderForChildStdOutput();
            void handleMessage(const boost::system::error_code& ec, std::size_t size);
            ProcessResult waitChildProcessToFinish();
            std::future<ProcessResult> asyncWaitChildProcessToFinish();
            void cacheResult();
            void cacheResultLocked(std::lock_guard<std::mutex>& locked);
            /* necessary for boost */
            std::string m_path;
            boost::asio::io_service asioIOService;
            std::vector<char> bufferForIOService;
            std::string m_output;
            boost::process::async_pipe asyncPipe;
            std::unique_ptr<boost::process::child, BoostChildProcessDestructor> m_child;
            int m_pid;

            /* handle the results */
            std::atomic<Process::ProcessStatus> m_status;
            Process::IProcess::functor m_callback;
            std::function<void(std::string)> m_notifyTrimmed;
            size_t m_outputLimit;
            std::atomic<bool> m_finished;
            std::shared_future<ProcessResult> m_result;
            ProcessResult m_cached;
            std::mutex m_onCacheResult;


        };
    }
}

