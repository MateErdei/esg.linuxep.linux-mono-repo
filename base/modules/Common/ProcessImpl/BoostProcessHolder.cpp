/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "BoostProcessHolder.h"

#include "IProcessException.h"
#include "Logger.h"

#include "../UtilityImpl/StringUtils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <boost/asio/read.hpp>
#include <boost/process/args.hpp>
#include <boost/process/env.hpp>
#include <boost/process/exe.hpp>
#include <boost/process/extend.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>
#include <boost/system/error_code.hpp>

#pragma GCC diagnostic pop

namespace
{
    std::vector<int> getFileDescriptorsToCloseAfterFork(const std::vector<int>& preserve)
    {
        std::vector<int> fds;
        struct stat buf;
#ifdef OPEN_MAX
        for (int fd = 3; fd < OPEN_MAX; fd++)
#else
        for (int fd = 3; fd < 256; ++fd)
#endif
        {
            if (::fstat(fd, &buf) != -1)
            {
                if (std::find(preserve.begin(), preserve.end(), fd) == preserve.end())
                {
                    fds.push_back(fd);
                }
            }
        }
        return fds;
    }

    class ScopedDeferExecution
    {
        std::function<void()> m_func;

    public:
        ScopedDeferExecution(std::function<void()> func) : m_func(std::move(func)) {}
        ~ScopedDeferExecution()
        {
            try
            {
                m_func();
            }
            catch (std::exception& ex)
            {
                LOGWARN("Exception on running deferred cleanup: " << ex.what());
            }
        }
    };

} // namespace

namespace Common
{
    namespace ProcessImpl
    {
        struct sophos_exe : boost::process::extend::handler
        {
        private:
            uid_t m_uid;
            gid_t m_gid;
            std::vector<int> m_fds{};

        public:
            sophos_exe(uid_t uid, gid_t gid, std::vector<int> fds) : m_uid(uid), m_gid(gid), m_fds(fds) {}

            template<typename Sequence>
            void on_exec_setup(boost::process::extend::posix_executor<Sequence>& exec)
            {

                // Must set groups first whilst still root
                std::string userName = Common::FileSystem::FilePermissionsImpl().getUserName(m_uid);

                if (::getuid() == 0) // if running as root.
                {
                    if (::initgroups(userName.c_str(), m_gid) != 0)
                    {
                        exec.set_error(boost::process::detail::get_last_error(), "initgroups failed for: " + userName + " with errno " + std::to_string(errno));
                        return;
                    }
                }

                if (::setgid(m_gid) != 0)
                {
                    exec.set_error(boost::process::detail::get_last_error(), "Failed to set group ids");
                    return;
                }

                if (::setuid(m_uid) != 0)
                {
                    exec.set_error(boost::process::detail::get_last_error(), "Failed to set user id");
                    return;
                }

                int previousErr = errno;
                for (auto& fd : m_fds)
                {
                    ::close(fd);
                }
                errno = previousErr;
            }
        };

        void BoostProcessHolder::armAsyncReaderForChildStdOutput()
        {
            boost::asio::async_read(
                asyncPipe,
                boost::asio::buffer(m_bufferForIOService),
                [this](const boost::system::error_code& ec, std::size_t size) -> std::size_t {
                    return this->completionCondition(ec, size);
                },
                [this](const boost::system::error_code& ec, std::size_t size) {
                    return this->handleMessage(ec, size);
                });
        }

        /** Method that is executed when the asio io service detects that either an error occurred or the
         *  given buffer was full.
         * https://www.boost.org/doc/libs/1_71_0/doc/html/boost_process/tutorial.html#boost_process.tutorial.async_io
         * @param ec : error case ( or end of file)
         * @param size : number of bytes written to the buffer.
         */
        void BoostProcessHolder::handleMessage(const boost::system::error_code& ec, std::size_t size)
        {
            std::lock_guard<std::mutex> lock{ m_outputAccess };
            if (ec)
            {
                // end of file ( pipe closed) is a normal thing to happen, no error at all.
                if (ec.value() != boost::asio::error::eof)
                {
                    LOGWARN("Message Output failed with error: " << ec.message());
                }
                // on error or when the message finished, there is no need to count the
                // buffer, hence, the output is just updated.
                if (size > 0)
                {
                    m_output +=
                        std::string(this->m_bufferForIOService.begin(), this->m_bufferForIOService.begin() + size);
                }
            }
            else
            {
                if (size > 0)
                {
                    // there is two cases to consider here:
                    // if m_outputLimit was set to 0 or not as it will require the execution of notified trimmed
                    if (m_outputLimit == 0)
                    {
                        m_output +=
                            std::string(this->m_bufferForIOService.begin(), this->m_bufferForIOService.begin() + size);
                    }
                    else
                    {
                        // the value of the m_outputLimit has also been given to the buffer, hence, this method will
                        // be executed when size is equal the m_outputLimit.
                        // the promise of the set of output limit is that there will be notification of past strings
                        // in such a way to avoid the remainder of the output to go beyond 2xm_outputLimit
                        if (!m_output.empty())
                        {
                            try
                            {
                                LOGDEBUG("Notify trimmed output");
                                m_notifyTrimmed(m_output);
                            }
                            catch (std::exception& ex)
                            {
                                LOGWARN("Exception on notify output trimmed: " << ex.what());
                            }
                        }
                        // notice the absence of += the output has been assigned the latest value in the
                        // m_bufferForIOService.
                        m_output =
                            std::string(this->m_bufferForIOService.begin(), this->m_bufferForIOService.begin() + size);

                        if (shouldBufferBeFlushed(size))
                        {
                            m_notifyTrimmed(m_output);
                            m_output = "";
                        }
                    }
                }
                // boost asio requires that async_read is armed again when the buffer has already been used.
                // this is only applied when there is no error.
                armAsyncReaderForChildStdOutput();
            }
        }

        ProcessResult BoostProcessHolder::waitChildProcessToFinish()
        {
            ScopedDeferExecution setStatusToFinished([this]() { this->m_status = Process::ProcessStatus::FINISHED; });
            try
            {
                LOGDEBUG("Process main loop: Check output");
                // there is one possible case where the ioService may hang forever ( for example, if the child receives
                // a sigkill. This is because the pipe will not be closed.
                // for this case, the io run in its thread, and after the recognition of the service stop
                // the pipe is explicitly closed.
                auto ioservice = std::async(std::launch::async, [this]() { asioIOService.run(); });
                LOGDEBUG(m_path << " Process main loop: Waiting for process to finish");
                std::error_code ec;
                m_child->wait(ec);
                LOGDEBUG(m_path << " Process main loop: Wait finished");
                try
                {
                    // give some extra time to the ioservice to capture any remaining data
                    if (ioservice.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
                    {
                        asyncPipe.close();
                    }
                }
                catch (std::exception& ex)
                {
                    LOGWARN("Error on closing the pipe: " << ex.what());
                }

                if (ec)
                {
                    LOGWARN("Process wait reported error: " << ec.message());
                }
                ioservice.get();
                LOGDEBUG("Process main loop: Retrieve results");
                ProcessResult result;
                {
                    std::lock_guard<std::mutex> lock{ m_outputAccess };
                    result.output = m_output;
                }
                result.exitCode = m_child->exit_code();
                try
                {
                    m_callback();
                }
                catch (std::exception& ex)
                {
                    LOGWARN("Exception on reporting process finished: " << ex.what());
                }
                return result;
            }
            catch (std::exception& ex)
            {
                LOGWARN(m_path << " Exception on the main loop for wait " << ex.what());
                throw;
            }
        }

        std::future<ProcessResult> BoostProcessHolder::asyncWaitChildProcessToFinish()
        {
            m_status = Process::ProcessStatus::RUNNING;
            return std::async(std::launch::async, [this]() { return this->waitChildProcessToFinish(); });
        }

        void BoostProcessHolder::cacheResult()
        {
            LOGDEBUG("Entering cache result");
            std::unique_lock<std::timed_mutex> lock{ m_onCacheResult };
            cacheResultLocked(lock);
            LOGDEBUG("Leaving cache result");
        }
        void BoostProcessHolder::cacheResultLocked(std::unique_lock<std::timed_mutex>& /*locked*/)
        {
            if (!m_finished)
            {
                try
                {
                    m_cached = m_result.get();
                }
                catch (std::exception& ex)
                {
                    LOGWARN("Exception on retrieving the result from process: " << ex.what());
                    m_cached.exitCode = 1;
                    m_cached.output = ex.what();
                }

                m_finished = true;
            }
        }

        std::size_t BoostProcessHolder::completionCondition(const boost::system::error_code& ec, std::size_t size)
        {
            if (ec.value() != 0)
            {
                return 0;
            }

            if (size == 0)
            {
                return m_outputLimit == 0 ? m_bufferForIOService.size() : m_outputLimit;
            }

            if (shouldBufferBeFlushed(size))
            {
                return 0;
            }

            return m_outputLimit == 0 ? m_bufferForIOService.size() : m_outputLimit;
        }

        BoostProcessHolder::BoostProcessHolder(
            const std::string& path,
            const std::vector<std::string>& arguments,
            const std::vector<Process::EnvironmentPair>& extraEnvironment,
            uid_t uid,
            gid_t gid,
            Process::IProcess::functor callback,
            std::function<void(std::string)> notifyTrimmed,
            size_t outputLimit,
            bool flushOnNewLine) :
            m_callback(std::move(callback)),
            m_notifyTrimmed(std::move(notifyTrimmed)),
            m_path(path),
            m_bufferForIOService(outputLimit == 0 ? 4096 : outputLimit),
            asyncPipe(asioIOService),
            m_status{ Process::ProcessStatus::NOTSTARTED },
            m_outputLimit(outputLimit),
            m_finished(false),
            m_enableBufferFlushOnNewLine(flushOnNewLine)
        {
            // the creation of child involves fork and must be serialized (no data race should occur at that time).
            // this is the reason for the static mutex around here.
            static std::mutex mutex;
            std::lock_guard<std::mutex> lock{ mutex };
            auto env = boost::this_process::environment();
            boost::process::environment env_ = env;
            for (auto& entry : extraEnvironment)
            {
                if (entry.first.empty())
                {
                    throw Common::Process::IProcessException("Environment name cannot be empty: '' = " + entry.second);
                }

                env_[entry.first] = entry.second;
            }
            int source = asyncPipe.native_source();
            int sink = asyncPipe.native_sink();
            auto fds = getFileDescriptorsToCloseAfterFork({ source, sink });
            m_child = std::unique_ptr<boost::process::child, BoostChildProcessDestructor>(new boost::process::child(
                boost::process::exe = path,
                boost::process::args = arguments,
                env_,
                (boost::process::std_out & boost::process::std_err) > asyncPipe,
                sophos_exe(uid, gid, fds)));
            LOGDEBUG("Child created");
            armAsyncReaderForChildStdOutput();
            LOGDEBUG("Monitoring of io in place");

            m_result = asyncWaitChildProcessToFinish();
            LOGDEBUG("Process Running");
            m_pid = m_child->id();
        }

        BoostProcessHolder::~BoostProcessHolder()
        {
            try
            {
                if (m_child->valid())
                {
                    m_child->terminate();
                }
                std::unique_lock<std::timed_mutex> lock{ m_onCacheResult };
                cacheResultLocked(lock);
            }
            catch (std::exception& ex)
            {
                LOGWARN("Exception in the destructor of process impl: " << ex.what());
            }
        }

        int BoostProcessHolder::pid() { return m_pid; }

        void BoostProcessHolder::wait() { cacheResult(); }

        Process::ProcessStatus BoostProcessHolder::wait(std::chrono::milliseconds timeToWait)
        {
            if (hasFinished())
            {
                return Process::ProcessStatus::FINISHED;
            }
            else
            {
                std::unique_lock<std::timed_mutex> lock{ m_onCacheResult, std::defer_lock };
                if (!lock.try_lock_for(timeToWait))
                {
                    return Process::ProcessStatus::TIMEOUT;
                }
                if (m_result.wait_for(timeToWait) == std::future_status::ready)
                {
                    cacheResultLocked(lock);
                    return Process::ProcessStatus::FINISHED;
                }
                return Process::ProcessStatus::TIMEOUT;
            }
        }

        int BoostProcessHolder::exitCode()
        {
            cacheResult();
            return m_cached.exitCode;
        }

        std::string BoostProcessHolder::output()
        {
            cacheResult();
            return m_cached.output;
        }

        bool BoostProcessHolder::hasFinished() { return m_status == Process::ProcessStatus::FINISHED; }

        void BoostProcessHolder::sendTerminateSignal()
        {
            if (hasFinished())
            {
                return;
            }
            LOGSUPPORT("Terminating process " << m_pid);
            ::kill(m_pid, SIGTERM);
        }

        void BoostProcessHolder::kill()
        {
            if (hasFinished())
            {
                return;
            }
            LOGDEBUG("Killing process " << m_pid);
            m_child->terminate();
            cacheResult();
        }

        bool BoostProcessHolder::shouldBufferBeFlushed(std::size_t size)
        {
            if (m_enableBufferFlushOnNewLine)
            {
                return std::find(m_bufferForIOService.begin(), m_bufferForIOService.begin() + size, '\n') !=
                       m_bufferForIOService.end();
            }
            return false;
        }

    } // namespace ProcessImpl
} // namespace Common
