/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessImpl.h"

#include "ArgcAndEnv.h"
#include "IProcessException.h"
#include "Logger.h"
#include "PipeHolder.h"
#include "StdPipeThread.h"
#include "../../../redist/boost/include/boost/process/child.hpp"
#include "../../../redist/boost/include/boost/process/args.hpp"
#include "../../../redist/boost/include/boost/system/error_code.hpp"

#include <Common/UtilityImpl/StringUtils.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <grp.h>
#include <iostream>
#include <pwd.h>
#include <queue>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <wait.h>

#include <boost/process/child.hpp>
#include <boost/process/args.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/env.hpp>
#include <boost/process/io.hpp>
#include <boost/process/extend.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/system/error_code.hpp>
#include <future>

std::unique_ptr<Common::Process::IProcess> Common::Process::createProcess()
{
    return ProcessImpl::ProcessFactory::instance().createProcess();
}

Common::Process::Milliseconds Common::Process::milli(int n_ms)
{
    return std::chrono::milliseconds(n_ms);
}

namespace Common
{
    namespace ProcessImpl
    {
        struct sophos_exe : boost::process::extend::handler
        {
            uid_t m_uid; gid_t m_gid;
            std::vector<int> m_fds{};
        public:
            sophos_exe(uid_t uid, gid_t gid): m_uid(uid), m_gid(gid){}

            template<typename Sequence>
            void on_exec_setup(boost::process::extend::posix_executor<Sequence> & exec)
            {
                // Must set group first whilst still root
                if (::setgid(m_gid) != 0)
                {
                    exec.set_error( boost::process::detail::get_last_error(), "Failed to set group id");
                    return;
                }
                if (::setuid(m_uid) != 0)
                {
                    exec.set_error( boost::process::detail::get_last_error(), "Failed to set user id");
                    return;
                }
            }

        };

        struct ProcessResult
        {
            int exitCode;
            std::string output;
        };

        /* In order to keep previous behaviour of ProcessImpl that would throw for some case in ::exec and not throw in
         * other cases of failure. And in order to avoid to have to keep track of the state of the process in the ProcessImpl,
         * there are two 'dummy' IProcessImpl
         *
         * Failed to StartProcess keep track of when the exec failed.
         */
        class FailedToStartProcess : public IProcessImpl
        {
        public:
            int pid() override {return -1; }
            void wait() override {return; }
            Process::ProcessStatus wait( std::chrono::milliseconds ) override
            {
                return Process::ProcessStatus::FINISHED;
            };
            int exitCode() override {return 2; }
            std::string output() override{ return ""; }
            bool hasFinished() override {return true; }
            void sendTerminateSignal() override {return; }
            void kill() override {return; }
        };

        /* Initial state, keep track that no exec was ever called */
        class NoExecCalled : public IProcessImpl
        {
        public:
            int pid() override {return -1; }
            void wait() override {return; }
            Process::ProcessStatus wait( std::chrono::milliseconds ) override
            {
                return Process::ProcessStatus::NOTSTARTED;
            };
            int exitCode() override {
                throw Process::IProcessException("Exit Code can be called only after exec and process exit.");
            }
            std::string output() override{
                throw Process::IProcessException("Output can be called only after exec.");
            }
            bool hasFinished() override {return true; }
            void sendTerminateSignal() override {return; }
            void kill() override {return; }
        };
        /**
         * This is the main object that implements the IprocessImpl interface using boost::process.
         */
        class ProcessImplOnBoost : public IProcessImpl
        {
            /* necessary for boost */
            boost::asio::io_service asioIOService;
            std::vector<char> bufferForIOService;
            std::string m_output;
            boost::process::async_pipe asyncPipe;
            std::unique_ptr<boost::process::child> m_child;

            /* handle the results */
            Process::IProcess::functor m_callback;
            std::function<void(std::string)> m_notifyTrimmed;
            size_t m_outputLimit;
            std::atomic<bool> m_finished;
            std::future<ProcessResult> m_result;
            ProcessResult m_cached;
            std::mutex m_onCacheResult;

            void armAsyncReaderForChildStdOutput()
            {
                boost::asio::async_read(asyncPipe, boost::asio::buffer(bufferForIOService), [this](const boost::system::error_code &ec, std::size_t size)
                {return this->handleMessage(ec,size);  } );
            }

            /** Method that is executed when the asio io service detects that either an error occurred or the
             *  given buffer was full.
             * https://www.boost.org/doc/libs/1_71_0/doc/html/boost_process/tutorial.html#boost_process.tutorial.async_io
             * @param ec : error case ( or end of file)
             * @param size : number of bytes written to the buffer.
             */
            void handleMessage( const boost::system::error_code &ec, std::size_t size )
            {
                if (ec)
                {
                    // end of file ( pipe closed) is a normal thing to happen, no error at all.
                    if ( ec.value() != boost::asio::error::eof)
                    {
                        LOGWARN("Message Output failed with error: " << ec.message());
                    }
                    // on error or when the message finished, there is no need to count the
                    // buffer, hence, the output is just updated.
                    if( size > 0 )
                    {
                        m_output+= std::string(this->bufferForIOService.begin(), this->bufferForIOService.begin()+size);
                    }
                }
                else
                {
                    if( size > 0 )
                    {
                        // there is two cases to consider here:
                        // if m_outputLimit was set to 0 or not as it will require the execution of notified trimmed
                        if( m_outputLimit == 0 )
                        {
                            m_output+= std::string(this->bufferForIOService.begin(), this->bufferForIOService.begin()+size);
                        }
                        else
                        {
                            // the value of the m_outputLimit has also been given to the buffer, hence, this method will
                            // be executed when size is equal the m_outputLimit.
                            // the promise of the set of output limit is that there will be notification of past strings
                            // in such a way to avoid the remainder of the output to go beyond 2xm_outputLimit
                            if ( m_output.size() > 0 )
                            {
                                try
                                {
                                    LOGDEBUG("Notify trimmed output");
                                    m_notifyTrimmed(m_output);
                                }catch ( std::exception & ex)
                                {
                                    LOGWARN("Exception on notify output trimed: "<< ex.what());
                                }

                            }
                            // notice the absence of += the output has been assigned the latest value in the bufferForIOServicefer.
                            m_output = std::string(this->bufferForIOService.begin(), this->bufferForIOService.begin()+size);
                        }
                    }
                    // boost asio requires that async_read is armed again when the buffer has already been used.
                    // this is only applied when there is no error.
                    armAsyncReaderForChildStdOutput();
                }

            }

            ProcessResult waitChildProcessToFinish()
            {
                LOGDEBUG("Process main loop: Check output");
                // there is one possible case where the ioService may hang forever ( for example, if the child receives
                // a sigkill. This is because the pipe will not be closed.
                // for this case, the io run in its thread, and after the recognition of the service stop
                // the pipe is explicitly closed.
                auto ioservice = std::async(std::launch::async, [this](){asioIOService.run();});
                LOGDEBUG("Process main loop: Wait process to finish");
                std::error_code ec;
                m_child->wait(ec);
                LOGDEBUG("Process main loop: Wait finished");
                try
                {
                    asyncPipe.close();
                }catch (std::exception& ex)
                {
                    LOGWARN("Error on closing the pipe: " << ex.what());
                }

                if( ec)
                {
                    LOGWARN("Process wait reported error: " << ec.message());
                }
                ioservice.get();
                LOGDEBUG("Process main loop: Retrieve results");
                ProcessResult result;
                result.output = m_output;
                result.exitCode = m_child->exit_code();
                try{
                    m_callback();
                }catch (std::exception & ex)
                {
                    LOGWARN("Exeption on reporting process finished: " << ex.what());
                }
                return result;
            }

            std::future<ProcessResult> asyncWaitChildProcessToFinish()
            {
                return std::async(std::launch::async, [this](){return this->waitChildProcessToFinish(); });
            }

            void cacheResult()
            {
                std::lock_guard<std::mutex> lock{m_onCacheResult};
                if( !m_finished)
                {
                    try
                    {
                        m_cached = m_result.get();
                    }catch (std::exception & ex)
                    {
                        LOGWARN("Exception on retrieving the result from process: " << ex.what());
                        m_cached.exitCode = 1;
                        m_cached.output = ex.what();
                    }

                    m_finished = true;
                }
            }

        public:
            ProcessImplOnBoost( const std::string& path,
                                const std::vector<std::string>& arguments,
                                const std::vector<Process::EnvironmentPair>& extraEnvironment,
                                uid_t uid,
                                gid_t gid,
                                Process::IProcess::functor callback,
                                std::function<void(std::string)> notifyTrimmed, size_t outputLimit):
                                bufferForIOService(outputLimit==0?4096:outputLimit),
                                asyncPipe(asioIOService),
                                m_callback(callback),
                                m_notifyTrimmed(notifyTrimmed),
                                m_outputLimit(outputLimit),
                                m_finished(false)
            {
                // the creation of child involves fork and must be serialized (no data race should occur at that time).
                // this is the reason for the static mutex around here.
                static std::mutex mutex;
                std::lock_guard<std::mutex> lock{mutex};
                auto env = boost::this_process::environment();
                boost::process::environment env_ = env;
                for( auto & entry: extraEnvironment)
                {
                    if (entry.first.empty())
                    {
                        throw Common::Process::IProcessException(
                                "Environment name cannot be empty: '' = " + entry.second);
                    }

                    env_[entry.first] = entry.second;
                }

                m_child = std::unique_ptr<boost::process::child>(new boost::process::child(
                        path, boost::process::args=arguments, env_, (boost::process::std_out & boost::process::std_err) > asyncPipe,
                        sophos_exe(uid, gid)
                        ));
                LOGDEBUG("Child created");
                armAsyncReaderForChildStdOutput();
                LOGDEBUG("Monitoring of io in place");

                m_result = asyncWaitChildProcessToFinish();
                LOGDEBUG("Process Running");
            }

            ~ProcessImplOnBoost()
            {
                try
                {
                    if( !hasFinished())
                    {
                        kill();
                    }
                    m_child->wait();

                }catch(std::exception& ex)
                {
                    LOGWARN("Exception in the destructor of process impl: "<< ex.what());
                }
            }

            int pid()
            {
                return m_child->id();
            }

            void wait()
            {
                cacheResult();
            }

            Process::ProcessStatus wait( std::chrono::milliseconds timeToWait)
            {
                if( m_finished)
                {
                    return Process::ProcessStatus::FINISHED;
                }
                else
                {
                    if( m_result.wait_for(timeToWait) == std::future_status::ready)
                    {
                        cacheResult();
                        return Process::ProcessStatus::FINISHED;
                    }
                    return Process::ProcessStatus::TIMEOUT;
                }
            }

            int exitCode()
            {

                cacheResult();
                return m_cached.exitCode;
            }

            std::string output()
            {
                cacheResult();
                return m_cached.output;
            }

            bool hasFinished()
            {
                return m_finished;
            }

            void sendTerminateSignal()
            {
                auto m_pid = pid();
                LOGSUPPORT("Terminating process " << m_pid);
                ::kill(m_pid, SIGTERM);
            }

            void kill()
            {
                auto m_pid = pid();
                LOGSUPPORT("Killing process " << m_pid);
                m_child->terminate();
            }
        };

        ProcessImpl::ProcessImpl() :
                m_pid{-1},
            m_outputLimit(0),
            m_callback{ []() {} },
        m_notifyTrimmed{ [](std::string) {} }
        {
            m_d.reset( new NoExecCalled());
        }

        ProcessImpl::~ProcessImpl() = default;

        Process::ProcessStatus ProcessImpl::wait(Process::Milliseconds period, int attempts)
        {
            Process::Milliseconds fullPeriod = period * attempts;
            if( fullPeriod < Process::Milliseconds{1})
            {
                fullPeriod = Process::Milliseconds{1};
            }
            auto processOnBoost = safeAccess();
            return processOnBoost->wait(fullPeriod);
        }

        void ProcessImpl::exec(const std::string& path, const std::vector<std::string>& arguments)
        {
            this->exec(path, arguments, std::vector<Process::EnvironmentPair>{}, ::getuid(), ::getgid());
        }

        void ProcessImpl::exec(
            const std::string& path,
            const std::vector<std::string>& arguments,
            const std::vector<Process::EnvironmentPair>& extraEnvironment)
        {
            this->exec(path, arguments, extraEnvironment, ::getuid(), ::getgid());
        }

        void ProcessImpl::exec(
            const std::string& path,
            const std::vector<std::string>& arguments,
            const std::vector<Process::EnvironmentPair>& extraEnvironment,
            uid_t uid,
            gid_t gid)
        {
            std::lock_guard<std::mutex> lock(m_protectImplOnBoost);
            m_pid = -1;
            try
            {
                m_d = std::shared_ptr<IProcessImpl>(new ProcessImplOnBoost(path, arguments, extraEnvironment, uid, gid, m_callback, m_notifyTrimmed,
                                               m_outputLimit));
                m_pid = m_d->pid();
            }
            catch (Common::Process::IProcessException& ex)
            {
                LOGWARN("Failure to setup process: " << ex.what());
                m_d = std::shared_ptr<IProcessImpl>(new FailedToStartProcess());
                throw;
            }catch ( std::exception& ex)
            {
                LOGWARN("Failure to start process: " << ex.what());
                m_callback();
                m_d = std::shared_ptr<IProcessImpl>(new FailedToStartProcess());
                //throw Process::IProcessException(ex.what());
            }
        }
        int ProcessImpl::exitCode()
        {
            auto processOnBoost = safeAccess();
            return processOnBoost->exitCode();
        }

        std::string ProcessImpl::output()
        {
            auto processOnBoost = safeAccess();
            return processOnBoost->output();
        }

        bool ProcessImpl::kill() { return kill(2); }

        bool ProcessImpl::kill(int secondsBeforeSIGKILL)
        {
            int numOfDecSeconds = secondsBeforeSIGKILL * 10;
            bool requiredKill = false;
            auto processOnBoost = safeAccess();
            if (!processOnBoost->hasFinished())
            {
                processOnBoost->sendTerminateSignal();
                if (wait(Process::milli(numOfDecSeconds), 100) == Process::ProcessStatus::TIMEOUT)
                {
                    processOnBoost->kill();
                    requiredKill = true;
                }
            }
            return requiredKill;
        }

        Process::ProcessStatus ProcessImpl::getStatus()
        {
            auto processOnBoost = safeAccess();
            if ( !processOnBoost)
            {
                LOGSUPPORT("getStatus can be called only after exec");
                return Process::ProcessStatus::NOTSTARTED;
            }

            if( processOnBoost->hasFinished())
            {
                return Process::ProcessStatus::FINISHED;
            }
            return Process::ProcessStatus::RUNNING;
        }

        void ProcessImpl::setOutputLimit(size_t limit)
        {
            m_outputLimit = limit;
        }

        void ProcessImpl::setNotifyProcessFinishedCallBack(Process::IProcess::functor callback)
        {
            m_callback = callback;
        }

        int ProcessImpl::childPid() const
        {
            return m_pid;
        }

        ProcessFactory::ProcessFactory() { restoreCreator(); }

        ProcessFactory& ProcessFactory::instance()
        {
            static ProcessFactory singleton;
            return singleton;
        }

        std::unique_ptr<Process::IProcess> ProcessFactory::createProcess() { return m_creator(); }

        void ProcessFactory::replaceCreator(std::function<std::unique_ptr<Process::IProcess>(void)> creator)
        {
            m_creator = std::move(creator);
        }

        void ProcessFactory::restoreCreator()
        {
            m_creator = []() {
                return std::unique_ptr<Common::Process::IProcess>(new Common::ProcessImpl::ProcessImpl());
            };
        }

        void ProcessImpl::waitUntilProcessEnds()
        {
            auto processOnBoost = safeAccess();
            processOnBoost->wait();
        }

        void ProcessImpl::setOutputTrimmedCallback(std::function<void(std::string)> outputTrimmedCallback)
        {
            m_notifyTrimmed = outputTrimmedCallback;
        }

        std::shared_ptr<IProcessImpl> ProcessImpl::safeAccess()
        {
            std::lock_guard<std::mutex> lock(m_protectImplOnBoost);
            std::shared_ptr<IProcessImpl> copy = m_d;
            return copy;
        }

    } // namespace ProcessImpl
} // namespace Common
