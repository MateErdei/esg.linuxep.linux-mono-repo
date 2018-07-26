/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessImpl.h"
#include "IProcessException.h"

#include "Common/Threads/AbstractThread.h"
#include "ArgcAndEnv.h"

#include <queue>
#include <thread>
#include <sstream>
#include <mutex>
#include <iostream>
#include <algorithm>
#include <condition_variable>

#include <cassert>
#include <cstring>
#include <unistd.h>
#include <wait.h>


#define LOGERROR(x) std::cerr << x << '\n'; //NOLINT
namespace
{
    // ensure child process do not keep filedescriptors that is only for the parent.
    void closeFileDescriptors(const std::vector<int> & keepFds)
    {
        int fd;


#ifdef OPEN_MAX
        for (fd = 3; fd < OPEN_MAX ; fd++)
#else
        for (fd = 3; fd < 256 ; fd++)
#endif
        {
            if ( std::find( keepFds.begin(), keepFds.begin(), fd ) == keepFds.end())
            {
                close(fd);
            }

        }
    }

}


class FileDescriptorHolder
{
    int m_fd;
public:
    explicit FileDescriptorHolder( int fd )
    {
        m_fd = fd;
    }

    ~FileDescriptorHolder()
    {
        ::close(m_fd);
    }
    int  fileDescriptor()
    {
        return m_fd;
    }
};




std::unique_ptr<Common::Process::IProcess> Common::Process::createProcess()
{
    return ProcessImpl::ProcessFactory::instance().createProcess();
}

Common::Process::Milliseconds Common::Process::milli( int n_ms)
{
    return std::chrono::milliseconds(n_ms);
}

namespace Common
{
namespace ProcessImpl
{

    class PipeHolder
    {
    public:
        PipeHolder(): m_pipe{-1,-1}, m_pipeclosed{false,false}
        {
            if (pipe(m_pipe) < 0)
            {
                throw Common::Process::IProcessException("Pipe construction failure");
            }
            m_pipeclosed[PIPE_READ] = false;
            m_pipeclosed[PIPE_WRITE] = false;
        }
        ~PipeHolder()
        {
            if ( !m_pipeclosed[PIPE_READ] )
            {
                close(m_pipe[PIPE_READ]);
            }
            if ( !m_pipeclosed[PIPE_WRITE] )
            {
                close(m_pipe[PIPE_WRITE]);
            }
        }
        int readFd()
        {
            assert( !m_pipeclosed[PIPE_READ]);
            return m_pipe[PIPE_READ];
        }
        int writeFd()
        {
            assert( !m_pipeclosed[PIPE_WRITE]);
            return m_pipe[PIPE_WRITE];

        }
        void closeRead()
        {
            assert( !m_pipeclosed[PIPE_READ]);
            close(m_pipe[PIPE_READ]);
            m_pipeclosed[PIPE_READ] = true;
        }

        void closeWrite()
        {
            assert( !m_pipeclosed[PIPE_WRITE]);
            close(m_pipe[PIPE_WRITE]);
            m_pipeclosed[PIPE_WRITE] = true;
        }


    private:
        int m_pipe[2];
        bool m_pipeclosed[2];
        constexpr static int PIPE_READ = 0;
        constexpr static int PIPE_WRITE = 1;


    };


    // Pipe has a limited capacity (see man 7 pipe : Pipe Capacity)
    // This class allows the pipe buffer to be cleared to ensure that child process is never blocked trying
    // to write to stdout or stderr.
    class StdPipeThread : public Common::Threads::AbstractThread
    {
    private:
        int m_fileDescriptor;
        std::stringstream m_stdoutStream;
        std::mutex m_mutex;
    public:
        /**
         *
         * @param fileDescriptor BORROWED fileDescriptor
         */
        explicit StdPipeThread(int fileDescriptor) :
                Common::Threads::AbstractThread()
                , m_fileDescriptor(fileDescriptor)
                , m_mutex()
        {

        }
        ~StdPipeThread() override  = default;

        std::string output()
        {
            hasFinished();
            return m_stdoutStream.str();
        }
        bool hasFinished()
        {
            // use lock in order to ensure ::run has finished. Hence, child process finished.
            std::unique_lock<std::mutex> lock(m_mutex);
            return true;
        }

    private:
        void run() override
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            announceThreadStarted();

            char buffer[101];
            while (! stopRequested())
            {
                ssize_t nread = read(m_fileDescriptor, buffer, 100);
                if ( nread == 0 )
                {
                    // this happens when the file descriptor is closed (on child exit)
                    // hence also means process finished.
                    return;
                }
                if ( nread == -1 )
                {
                    int err = errno;
                    LOGERROR( "Error reading from pipe: "<<err<<": " << ::strerror(err));
                }
                buffer[nread] = '\0';
                m_stdoutStream << buffer;
            }
        }

    };




    ProcessImpl::ProcessImpl(): m_exitcode(std::numeric_limits<int>::max())
    {

    }

    ProcessImpl::~ProcessImpl() = default;

    Process::ProcessStatus ProcessImpl::wait(Process::Milliseconds period, int attempts)
    {
        assert( attempts>0);
        if ( m_exitcode != std::numeric_limits<int>::max())
        {
            return Process::ProcessStatus::FINISHED;
        }
        for ( int i = 0; i< attempts + 1; i++ )
        {
            // looping through attempts + 1 and skipping first sleep
            // ensure we do all the attempts
            // but do not sleep when it is not needed ( process already finished)
            if( i!= 0)
            {
                std::this_thread::sleep_for(period);
            }

            int status;
            pid_t ret = waitpid(m_pid, &status, WNOHANG);

            if (ret != 0)
            {
                m_pipeThread->requestStop();


                if (WIFEXITED(status)) //NOLINT
                {
                    m_exitcode = WEXITSTATUS(status); //NOLINT
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
        this->exec(path, arguments, std::vector<Process::EnvironmentPair>{});
    }


    void ProcessImpl::exec(const std::string &path, const std::vector<std::string> &arguments,
                           const std::vector<Process::EnvironmentPair> &extraEnvironment)
    {

        if (m_pipeThread)
        {
            m_pipeThread->requestStop();
        }
        m_pipe.reset(new PipeHolder());
        m_pipeThread.reset();

        ArgcAndEnv argcAndEnv(path, arguments, extraEnvironment);

        std::vector<int> fileDescriptorsToPreserveAfterFork({m_pipe->writeFd()});

        pid_t child = fork();

        switch(child)
        {
            case -1:
            {

                std::ostringstream ost;
                ost << "fork() failed, errno=" << errno;
                throw Process::IProcessException( ost.str());
            }
            //child
            case 0:

                closeFileDescriptors(fileDescriptorsToPreserveAfterFork);


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
                if ( extraEnvironment.empty())
                {
                    (void) execv(argcAndEnv.path(), argcAndEnv.argc());
                }
                else
                {
                    (void) execvpe(argcAndEnv.path(), argcAndEnv.argc(), argcAndEnv.envp());
                }
                // if reaches this point, execv failed
                // for normal execution, execv never returns.
                _exit(errno);

                //break;

            default:
                break;

        }

        // parent
        // close the write pipe as parent wants to read what the child writes to stdout, stderr
        m_pipe->closeWrite();
        m_pid = child;

        m_pipeThread.reset( new StdPipeThread(m_pipe->readFd()));
        m_pipeThread->start();
    }

    int ProcessImpl::exitCode()
    {
        if ( m_pipeThread)
        {
            if ( m_pipeThread->hasFinished())
            {
                if ( m_exitcode == std::numeric_limits<int>::max())
                {
                    for ( int i = 0; i<10; i++)
                    {
                        // get the exit code
                        wait(Process::milli(i), 1);
                        if ( m_exitcode != std::numeric_limits<int>::max())
                        {
                            break;
                        }
                    }
                }

                return m_exitcode;
            }
        }
        throw Process::IProcessException( "Exit Code can be called only after exec.");
    }

    std::string ProcessImpl::output()
    {
        if ( m_pipeThread)
        {
            return m_pipeThread->output();
        }
        throw Process::IProcessException( "Output can be called only after exec.");
    }

    void ProcessImpl::kill()
    {
        ::kill(m_pid, SIGTERM);
        if ( wait( Process::milli(2), 10) == Process::ProcessStatus::TIMEOUT)
        {
            ::kill(m_pid, SIGKILL);
        }

    }

    Process::ProcessStatus ProcessImpl::getStatus()
    {
        if (!m_pipeThread)
        {
            throw Process::IProcessException( "getStatus can be called only after exec.");
        }
        int status;
        pid_t exited = ::waitpid(m_pid, &status, WNOHANG);
        if (exited == 0)
        {
            return Process::ProcessStatus::RUNNING;
        }
        m_exitcode = status;
        return Process::ProcessStatus::FINISHED;
    }


    ProcessFactory::ProcessFactory()
    {
        restoreCreator();
    }

    ProcessFactory& ProcessFactory::instance()
    {
        static ProcessFactory singleton;
        return singleton;
    }

    std::unique_ptr<Process::IProcess> ProcessFactory::createProcess()
    {
        return m_creator();
    }

    void ProcessFactory::replaceCreator(std::function<std::unique_ptr<Process::IProcess>(void)> creator)
    {
        m_creator = std::move(creator);
    }

    void ProcessFactory::restoreCreator()
    {
        m_creator = [](){ return std::unique_ptr<Common::Process::IProcess>(new Common::ProcessImpl::ProcessImpl());  };
    }

}
}