/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessImpl.h"

#include "ArgcAndEnv.h"
#include "IProcessException.h"
#include "Logger.h"
#include "PipeHolder.h"
#include "StdPipeThread.h"

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

namespace
{
    // ensure child process do not keep filedescriptors that is only for the parent.
    void closeFileDescriptors(const std::vector<int>& keepFds)
    {
        int fd;

#ifdef OPEN_MAX
        for (fd = 3; fd < OPEN_MAX; fd++)
#else
        for (fd = 3; fd < 256; ++fd)
#endif
        {
            if (std::find(keepFds.begin(), keepFds.end(), fd) == keepFds.end())
            {
                close(fd);
            }
        }
    }

} // namespace

class FileDescriptorHolder
{
    int m_fd;

public:
    explicit FileDescriptorHolder(int fd) { m_fd = fd; }

    ~FileDescriptorHolder() { ::close(m_fd); }
    int fileDescriptor() { return m_fd; }
};

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
        ProcessImpl::ProcessImpl() :
            m_pid(-1),
            m_exitcode(std::numeric_limits<int>::max()),
            m_outputLimit(0),
            m_callback{ []() {} }
        {
        }

        ProcessImpl::~ProcessImpl() = default;

        Process::ProcessStatus ProcessImpl::wait(Process::Milliseconds period, int attempts)
        {
            assert(attempts > 0);
            if (m_exitcode != std::numeric_limits<int>::max())
            {
                return Process::ProcessStatus::FINISHED;
            }
            for (int i = 0; i < attempts + 1; i++)
            {
                // looping through attempts + 1 and skipping first sleep
                // ensure we do all the attempts
                // but do not sleep when it is not needed ( process already finished)
                if (i != 0)
                {
                    std::this_thread::sleep_for(period);
                }

                int status;
                pid_t ret = waitpid(m_pid, &status, WNOHANG);

                if (ret != 0)
                {
                    m_pid = -1;
                    m_pipeThread->requestStop();

                    if (WIFEXITED(status)) // NOLINT
                    {
                        m_exitcode = WEXITSTATUS(status); // NOLINT
                    }
                    else
                    {
                        // this happens when child was either killed, coredump.
                        // meaning that it is finished, but WIFEXITED does not return true.
                        m_exitcode = ECANCELED;
                    }

                    return Process::ProcessStatus::FINISHED;
                }
            }
            return Process::ProcessStatus::TIMEOUT;
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
            if (m_pid != -1)
            {
                kill();
            }

            if (m_pipeThread)
            {
                m_pipeThread->requestStop();
            }
            m_pipe.reset(new PipeHolder());
            m_pipeThread.reset();

            ArgcAndEnv argcAndEnv(path, arguments, extraEnvironment);

            std::vector<int> fileDescriptorsToPreserveAfterFork({ m_pipe->writeFd() });

            pid_t child = fork();

            switch (child)
            {
                case -1:
                {
                    std::ostringstream ost;
                    ost << "fork() failed, errno=" << errno;
                    throw Process::IProcessException(ost.str());
                }
                // child
                case 0:

                    closeFileDescriptors(fileDescriptorsToPreserveAfterFork);

                    // Must set group first whilst still root
                    if (::setgid(gid) != 0)
                    {
                        _exit(errno);
                    }
                    if (::setuid(uid) != 0)
                    {
                        _exit(errno);
                    }

                    // redirect stdout
                    if (dup2(m_pipe->writeFd(), STDOUT_FILENO) == -1)
                    {
                        _exit(errno);
                    }

                    // redirect stderr
                    if (dup2(m_pipe->writeFd(), STDERR_FILENO) == -1)
                    {
                        _exit(errno);
                    }
                    // close the read pipe as child is supposed to be only for writing (stderr,stdout)
                    m_pipe->closeRead();

                    if (extraEnvironment.empty())
                    {
                        (void)execv(argcAndEnv.path(), argcAndEnv.argc());
                    }
                    else
                    {
                        (void)execvpe(argcAndEnv.path(), argcAndEnv.argc(), argcAndEnv.envp());
                    }
                    // if reaches this point, execv failed
                    // for normal execution, execv never returns.
                    _exit(errno);

                    // break;

                default:
                    break;
            }

            // parent
            // close the write pipe as parent wants to read what the child writes to stdout, stderr
            m_pipe->closeWrite();
            m_pid = child;
            m_exitcode = std::numeric_limits<int>::max();

            m_pipeThread.reset(new StdPipeThread(m_pipe->readFd(), [this]() { onExecFinished(); }));
            m_pipeThread->setOutputLimit(m_outputLimit);
            m_pipeThread->start();
        }

        int ProcessImpl::exitCode()
        {
            if (m_exitcode != std::numeric_limits<int>::max())
            {
                // Already recorded the exit code
                return m_exitcode;
            }
            if (m_pipeThread)
            {
                // Need to stop the pipe thread to stop
                m_pipeThread->requestStop();
                if (m_pipeThread->hasFinished()) // waits for thread to finish
                {
                    // Now get the exit code from the process
                    if (m_exitcode == std::numeric_limits<int>::max())
                    {
                        for (int i = 0; i < 100; i++)
                        {
                            // get the exit code
                            wait(Process::milli(i), 1);
                            if (m_exitcode != std::numeric_limits<int>::max())
                            {
                                break;
                            }
                        }
                    }

                    return m_exitcode;
                }
            }
            // Not got a pipe, and no exit code, so we've never executed
            throw Process::IProcessException("Exit Code can be called only after exec and process exit.");
        }

        std::string ProcessImpl::output()
        {
            if (m_pipeThread)
            {
                return m_pipeThread->output(); // waits for thread to exit
            }
            throw Process::IProcessException("Output can be called only after exec.");
        }

        bool ProcessImpl::kill()
        {
            return kill(2);
        }

        bool ProcessImpl::kill(int secondsBeforeSIGKILL)
        {
            int numOfDecSeconds = secondsBeforeSIGKILL * 10;
            bool requiredKill = false;
            if (m_pid > 0)
            {
                LOGSUPPORT("Terminating process " << m_pid);
                ::kill(m_pid, SIGTERM);
                if (wait(Process::milli(numOfDecSeconds), 100) == Process::ProcessStatus::TIMEOUT)
                {
                    LOGSUPPORT("Killing process " << m_pid);
                    ::kill(m_pid, SIGKILL);
                    requiredKill = true;
                }
                m_pid = -1;
            }
            if (m_pipeThread)
            {
                m_pipeThread->requestStop();
            }
            return requiredKill;

        }


        Process::ProcessStatus ProcessImpl::getStatus()
        {
            if (!m_pipeThread)
            {
                throw Process::IProcessException("getStatus can be called only after exec.");
            }
            if (m_pid == -1)
            {
                return Process::ProcessStatus::FINISHED;
            }
            int status;
            pid_t exited = ::waitpid(m_pid, &status, WNOHANG);
            if (exited == 0)
            {
                return Process::ProcessStatus::RUNNING;
            }
            m_pid = -1;
            m_exitcode = status;
            m_pipeThread->requestStop(); // nothing more will be logged, so start the thread exiting
            return Process::ProcessStatus::FINISHED;
        }

        void ProcessImpl::setOutputLimit(size_t limit)
        {
            m_outputLimit = limit;
            if (m_pipeThread)
            {
                m_pipeThread->setOutputLimit(limit);
            }
        }

        void ProcessImpl::onExecFinished() { m_callback(); }

        void ProcessImpl::setNotifyProcessFinishedCallBack(Process::IProcess::functor callback)
        {
            m_callback = callback;
        }

        int ProcessImpl::childPid() const { return m_pid; }

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
            int status;
            pid_t ret = waitpid(m_pid, &status, 0);

            if (ret == -1)
            {
                LOGERROR("The PID " << m_pid << " does not exist or is not a child of the calling process.");
            }
            else
            {
                if (WIFEXITED(status))
                {
                    LOGDEBUG("PID " << m_pid << " exited, status=" << WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    LOGDEBUG("PID " << m_pid << " killed by signal, status=" << WTERMSIG(status));
                }
                // WIFSTOPPED can only occur if the call is done using WUNTRACED or if the child is being traced by ptrace
                else if (WIFSTOPPED(status)) {
                    LOGDEBUG("PID " << m_pid << " stopped by signal, status=" << WSTOPSIG(status));
                }
                else if (WIFCONTINUED(status)) {
                    LOGDEBUG("PID " << m_pid << " continued to run");
                }

            }
        }

    } // namespace ProcessImpl
} // namespace Common