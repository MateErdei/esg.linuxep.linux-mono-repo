// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "BoostProcessHolder.h"

#include "Logger.h"

#include "Common/Process/IProcessException.h"
#include "Common/UtilityImpl/StringUtils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "Common/FileSystem/IFilePermissions.h"

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

namespace Common::ProcessImpl
{
    struct sophos_exe : boost::process::extend::handler
    {
    private:
        std::string username_;
        uid_t m_uid;
        gid_t m_gid;
        std::vector<int> m_fds{};

    public:
        sophos_exe(uid_t uid, gid_t gid, std::vector<int> fds) : m_uid(uid), m_gid(gid), m_fds(std::move(fds))
        {
            username_ = Common::FileSystem::filePermissions()->getUserName(m_uid);
        }

        template<typename Sequence>
        void on_exec_setup(boost::process::extend::posix_executor<Sequence>& exec)
        {
            // Must set groups first whilst still root
            if (::getuid() == 0 && m_uid != 0) // if running as root.
            {
                if (::initgroups(username_.c_str(), m_gid) != 0)
                {
                    exec.set_error(
                        boost::process::detail::get_last_error(),
                        "initgroups failed for: " + username_ + " with errno " + std::to_string(errno));
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
            asyncPipeOut,
            boost::asio::buffer(m_bufferForIOServiceOut),
            [this](const boost::system::error_code& ec, std::size_t size) -> std::size_t
            { return this->completionCondition(ec, size,  m_bufferForIOServiceOut); },
            [this](const boost::system::error_code& ec, std::size_t size) { return this->handleOutMessage(ec, size); });
    }

    void BoostProcessHolder::armAsyncReaderForChildStdErr()
    {
        boost::asio::async_read(
            asyncPipeErr,
            boost::asio::buffer(m_bufferForIOServiceErr),
            [this](const boost::system::error_code& ec, std::size_t size) -> std::size_t
            { return this->completionCondition(ec, size,  m_bufferForIOServiceErr); },
            [this](const boost::system::error_code& ec, std::size_t size) { return this->handleErrMessage(ec, size); });
    }

    /** Method that is executed when the asio io service detects that either an error occurred or the
     *  given buffer was full.
     * https://www.boost.org/doc/libs/1_71_0/doc/html/boost_process/tutorial.html#boost_process.tutorial.async_io
     * @param ec : error case ( or end of file)
     * @param size : number of bytes written to the buffer.
     */

    void BoostProcessHolder::handleOutMessage(const boost::system::error_code& ec, std::size_t size)
    {
        std::lock_guard<std::mutex> lock{ m_outputAccess };
        if (ec)
        {
            // end of file ( pipe closed) is a normal thing to happen, no error at all.
            // When a process receives a SIGKILL, SIGABRT or SIGUSR1 the error code won't be eof as the pipe was not
            // closed normally (SIGTERM results in eof). But ec.value() will be operation_aborted and this is not an error
            if (ec.value() != boost::asio::error::eof && ec.value() != boost::asio::error::operation_aborted)
            {
                LOGWARN("Message Output failed with error: " << ec.message());
            }
            // on error or when the message finished, there is no need to count the
            // buffer, hence, the output is just updated.
            if (size > 0)
            {
                m_stdout +=
                    std::string(this->m_bufferForIOServiceOut.begin(), this->m_bufferForIOServiceOut.begin() + size);
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
                    m_stdout += std::string(
                        this->m_bufferForIOServiceOut.begin(), this->m_bufferForIOServiceOut.begin() + size);
                }
                else
                {
                    // the value of the m_outputLimit has also been given to the buffer, hence, this method will
                    // be executed when size is equal the m_outputLimit.
                    // the promise of the set of output limit is that there will be notification of past strings
                    // in such a way to avoid the remainder of the output to go beyond 2xm_outputLimit
                    if (!m_stdout.empty())
                    {
                        try
                        {
                            LOGDEBUG("Notify trimmed output");
                            m_notifyTrimmed(m_stdout);
                        }
                        catch (std::exception& ex)
                        {
                            LOGWARN("Exception on notify output trimmed: " << ex.what());
                        }
                    }
                    // notice the absence of += the output has been assigned the latest value in the
                    // m_bufferForIOServiceOut.
                    m_stdout = std::string(
                        this->m_bufferForIOServiceOut.begin(), this->m_bufferForIOServiceOut.begin() + size);

                    if (shouldBufferBeFlushed(size, m_bufferForIOServiceOut))
                    {
                        m_notifyTrimmed(m_stdout);
                        m_stdout = "";
                    }
                }
            }
            // boost asio requires that async_read is armed again when the buffer has already been used.
            // this is only applied when there is no error.
            armAsyncReaderForChildStdOutput();
        }
    }

    void BoostProcessHolder::handleErrMessage(const boost::system::error_code& ec, std::size_t size)
    {
        std::lock_guard<std::mutex> lock{ m_outputAccess };
        if (ec)
        {
            // end of file ( pipe closed) is a normal thing to happen, no error at all.
            // When a process receives a SIGKILL, SIGABRT or SIGUSR1 the error code won't be eof as the pipe was not
            // closed normally (SIGTERM results in eof). But ec.value() will be operation_aborted and this is not an error
            if (ec.value() != boost::asio::error::eof && ec.value() != boost::asio::error::operation_aborted)
            {
                LOGWARN("Message Output failed with error: " << ec.message());
            }
            // on error or when the message finished, there is no need to count the
            // buffer, hence, the output is just updated.
            if (size > 0)
            {
                m_stderr +=
                    std::string(this->m_bufferForIOServiceErr.begin(), this->m_bufferForIOServiceErr.begin() + size);
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
                    m_stderr += std::string(
                        this->m_bufferForIOServiceErr.begin(), this->m_bufferForIOServiceErr.begin() + size);
                }
                else
                {
                    // the value of the m_outputLimit has also been given to the buffer, hence, this method will
                    // be executed when size is equal the m_outputLimit.
                    // the promise of the set of output limit is that there will be notification of past strings
                    // in such a way to avoid the remainder of the output to go beyond 2xm_outputLimit
                    if (!m_stderr.empty())
                    {
                        try
                        {
                            LOGDEBUG("Notify trimmed error output");
                            m_notifyTrimmed(m_stderr);
                        }
                        catch (std::exception& ex)
                        {
                            LOGWARN("Exception on notify error output trimmed: " << ex.what());
                        }
                    }
                    // notice the absence of += the output has been assigned the latest value in the
                    // m_bufferForIOServiceErr.
                    m_stderr = std::string(
                        this->m_bufferForIOServiceErr.begin(), this->m_bufferForIOServiceErr.begin() + size);

                    if (shouldBufferBeFlushed(size, m_bufferForIOServiceErr))
                    {
                        m_notifyTrimmed(m_stderr);
                        m_stderr = "";
                    }
                }
            }
            // boost asio requires that async_read is armed again when the buffer has already been used.
            // this is only applied when there is no error.
            armAsyncReaderForChildStdErr();
        }
    }

    ProcessResult BoostProcessHolder::waitChildProcessToFinish()
    {
        ScopedDeferExecution setStatusToFinished([this]() { this->m_status = Process::ProcessStatus::FINISHED; });
        try
        {
            LOGDEBUG("Process main loop: Check output");
            // there is one possible case where the ioService may hang forever for example, if the child receives
            // a sigkill. This is because the pipe will not be closed.
            // for this case, the io run in its thread, and after the recognition of the service stop
            // the pipe is explicitly closed.
            auto ioservice = std::async(std::launch::async, [this]() { asioIOService.run(); });
            LOGDEBUG(m_path << " Process main loop: Waiting for process to finish");
            std::error_code ec;
            m_child->wait(ec);
            LOGDEBUG(m_path << " Process main loop: Wait finished");
            LOGDEBUG("Error code from wait: " << ec.value());

            try
            {
                // give some extra time to the ioservice to capture any remaining data
                // In the case of a SIGKILL, SIGABRT or SIGUSR1, the wait_for will wait the maximum amount seconds
                // But we shouldn't/don't need to wait for any amount, jut need to go ahead and close the pipes
                // ec.value() will be ECHILD in those cases so check for that
                if (ec.value() == ECHILD || ioservice.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
                {
                    asyncPipeErr.close();
                    asyncPipeOut.close();
                }
            }
            catch (std::exception& ex)
            {
                LOGWARN("Error on closing the pipe: " << ex.what());
            }

            if (ec)
            {
                if (isDeliberatelyKilled())
                {
                    LOGINFO("Process wait reported error after deliberately being terminated: " << ec.message());
                }
                else
                {
                    LOGWARN("Process wait reported error: " << ec.message());
                }
            }
            ioservice.get();
            LOGDEBUG("Process main loop: Retrieve results");
            ProcessResult result;
            {
                std::lock_guard<std::mutex> lock{ m_outputAccess };
                result.output = m_stdout;
                result.errorlog = m_stderr;
            }
            result.exitCode = m_child->exit_code();              // Signal overloaded with exit code
            result.nativeExitCode = m_child->native_exit_code(); // Separate signal and exit code
            /*
             * Use native_exit_code to allow us to differentiate signals and exit codes.
             * exit_code() is eval_exit_status(native_exit_code()):
             *
                    inline int eval_exit_status(int code)
                    {
                        if (WIFEXITED(code))
                        {
                            return WEXITSTATUS(code);
                        }
                        else if (WIFSIGNALED(code))
                        {
                            return WTERMSIG(code);
                        }
                        else
                        {
                            return code;
                        }
                    }
             */
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
                m_cached.exitCode = -255;
                m_cached.output = ex.what();
            }

            m_finished = true;
        }
    }

    std::size_t BoostProcessHolder::completionCondition(
        const boost::system::error_code& ec,
        std::size_t size,
        std::vector<char>& buffer)
    {
        if (ec.value() != 0)
        {
            return 0;
        }

        if (size == 0)
        {
            return m_outputLimit == 0 ? buffer.size() : m_outputLimit;
        }

        if (shouldBufferBeFlushed(size, buffer))
        {
            return 0;
        }

        return m_outputLimit == 0 ? buffer.size() : m_outputLimit;
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
        m_bufferForIOServiceErr(outputLimit == 0 ? 4096 : outputLimit),
        m_bufferForIOServiceOut(outputLimit == 0 ? 4096 : outputLimit),
        asyncPipeErr(asioIOService),
        asyncPipeOut(asioIOService),
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

        int sourceErr = asyncPipeErr.native_source();
        int sinkErr = asyncPipeErr.native_sink();
        int sourceOut = asyncPipeOut.native_source();
        int sinkOut = asyncPipeOut.native_sink();
        auto fds = getFileDescriptorsToCloseAfterFork({ sourceErr, sinkErr, sourceOut, sinkOut });
        m_child = std::unique_ptr<boost::process::child, BoostChildProcessDestructor>(new boost::process::child(
            boost::process::exe = path,
            boost::process::args = arguments,
            env_,

            boost::process::std_out > asyncPipeOut,
            boost::process::std_err > asyncPipeErr,
            sophos_exe(uid, gid, fds)));
        LOGDEBUG("Child created");

        armAsyncReaderForChildStdOutput();
        armAsyncReaderForChildStdErr();
        LOGDEBUG("Monitoring of io in place");

        m_result = asyncWaitChildProcessToFinish();
        m_pid = m_child->id();
        LOGDEBUG("Process Running " << m_pid);
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

    int BoostProcessHolder::pid()
    {
        return m_pid;
    }

    void BoostProcessHolder::wait()
    {
        cacheResult();
    }

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

    int BoostProcessHolder::nativeExitCode()
    {
        cacheResult();
        return m_cached.nativeExitCode;
    }

    std::string BoostProcessHolder::output()
    {
        cacheResult();
        if (m_cached.output.empty() && m_cached.errorlog.empty())
        {
            return "";
        }
        if (!m_cached.output.empty() && !m_cached.errorlog.empty())
        {
            return m_cached.output + "\n" + m_cached.errorlog;
        }
        else if (!m_cached.output.empty())
        {
            return m_cached.output;
        }
        else
        {
            return m_cached.errorlog;
        }
    }

    std::string BoostProcessHolder::stderroutput()
    {
        cacheResult();
        return m_cached.errorlog;
    }

    std::string BoostProcessHolder::stdoutput()
    {
        cacheResult();
        return m_cached.output;
    }

    bool BoostProcessHolder::hasFinished()
    {
        return m_status == Process::ProcessStatus::FINISHED;
    }

    void BoostProcessHolder::sendTerminateSignal()
    {
        if (hasFinished())
        {
            return;
        }
        LOGDEBUG("Terminating process " << m_pid);
        ::kill(m_pid, SIGTERM);
    }

    void BoostProcessHolder::sendAbortSignal()
    {
        if (hasFinished())
        {
            return;
        }
        setDeliberatelyKilled(true);
        LOGINFO("Killing process with abort signal " << m_pid);
        ::kill(m_pid, SIGABRT);
    }

    void BoostProcessHolder::sendUsr1Signal()
    {
        if (hasFinished())
        {
            return;
        }
        LOGINFO("Sending SIGUSR1 signal to process " << m_pid);
        ::kill(m_pid, SIGUSR1);
    }

    void BoostProcessHolder::kill()
    {
        if (hasFinished())
        {
            return;
        }
        setDeliberatelyKilled(true);
        LOGINFO("Killing process " << m_pid);
        m_child->terminate();
        cacheResult();
    }

    bool BoostProcessHolder::shouldBufferBeFlushed(std::size_t size, std::vector<char>& buffer)
    {
        if (m_enableBufferFlushOnNewLine)
        {
            return std::find(buffer.begin(), buffer.begin() + size, '\n') != buffer.end();
        }
        return false;
    }

    void BoostProcessHolder::setDeliberatelyKilled(bool value)
    {
        auto locked = deliberatelyKilled_.lock();
        *locked = value;
    }

    bool BoostProcessHolder::isDeliberatelyKilled()
    {
        const auto locked = deliberatelyKilled_.lock();
        return *locked;
    }
} // namespace Common::ProcessImpl
