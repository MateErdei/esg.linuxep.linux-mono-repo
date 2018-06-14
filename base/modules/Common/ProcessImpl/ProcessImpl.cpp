/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessImpl.h"
#include "IProcessException.h"
#include <memory>
#include <unistd.h>
#include <cassert>
#include <queue>
#include <thread>
#include <sstream>
#include <mutex>
#include <wait.h>
#include <signal.h>
#include <iostream>
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include "Common/Threads/AbstractThread.h"
#define LOGERROR(x) std::cerr << x << '\n';
namespace
{
    // ensure child process do not keep filedescriptors that is only for the parent.
    void closeFileDescriptors(std::vector<int> keepFds)
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
    FileDescriptorHolder( int fd )
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


/**
 * execve expects argc and envp to be given as char**.
 * This class allows the expected pointers to be passed to the execve without needing to handle
 * allocations and deallocations internally. It uses vector to handle all the allocations correctly.
 * It is safe do to so as execve will anyway put the arguments and enviroment variables in a 'safe place'
 * before changing the image of the process.
 */
class ArgcAndEnv
{
    class ArgcAdaptor
    {
    public:
        ArgcAdaptor(){};
        void setArgs( std::vector<std::string> arguments, std::string path = std::string() )
        {
            m_arguments.clear();
            if (!path.empty())
            {
                m_arguments.push_back(path);
            }
            for( auto & arg: arguments)
            {
                m_arguments.push_back(arg);
            }

            // const_cast is needed because execvp prototype wants an
            // array of char*, not const char*.
            for (auto const& arg : m_arguments)
            {
                m_argcAdaptor.emplace_back(const_cast<char *>(arg.c_str()));
            }
            // NULL terminate
            m_argcAdaptor.push_back(nullptr);

        }
        char** argcpointer()
        {
            if ( m_argcAdaptor.empty())
                return nullptr;
            return m_argcAdaptor.data();
        }
    private:
        std::vector<std::string> m_arguments;
        std::vector<char * > m_argcAdaptor;
    };
    ArgcAdaptor m_argc;
    ArgcAdaptor m_env;
    std::string m_path;
public:
    ArgcAndEnv(std::string path, std::vector<std::string> arguments,
               std::vector<Common::Process::EnvironmentPair> extraEnvironment)
    {
        m_path = path;
        std::vector<std::string> envArguments;
        for(auto & env : extraEnvironment)
        {
            // make sure all environment variables are valid.
            if(env.first.empty())
            {
                throw Common::Process::IProcessException("Environment name cannot be empty: '' = " + env.second);
            }
            envArguments.push_back(env.first + '=' + env.second);
        }
        m_argc.setArgs(arguments, path);
        m_env.setArgs(envArguments);
    }
    char ** argc()
    {
        m_argc.argcpointer();
    }
    char ** envp()
    {
        m_env.argcpointer();
    }
    char * path()
    {
        return const_cast<char *>( m_path.data());

    }
};

std::unique_ptr<Common::Process::IProcess> Common::Process::createProcess()
{
    return Common::Process::IProcessPtr(new Common::ProcessImpl::ProcessImpl());
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
        PipeHolder()
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
        explicit StdPipeThread(int fileDescriptor) :
                Common::Threads::AbstractThread()
                , m_fileDescriptor(fileDescriptor)
                , m_mutex()
        {

        }
        ~StdPipeThread()
        {
        }

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

            FileDescriptorHolder input( m_fileDescriptor);
            char buffer[101];
            while (! stopRequested())
            {
                size_t nread = read(input.fileDescriptor(), buffer, 100);
                if ( nread == 0 )
                {
                    // this happens when the file descriptor is closed (on child exit)
                    // hence also means process finished.
                    return;
                }
                if ( nread == -1 )
                {
                    int err = errno;
                    LOGERROR( ::strerror(err));
                }
                buffer[nread] = '\0';
                m_stdoutStream << buffer;
            }
        }

    };




    ProcessImpl::ProcessImpl(): m_exitcode(std::numeric_limits<int>::max())
    {

    }

    ProcessImpl::~ProcessImpl()
    {
    }

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
                std::cout << "status " << status << " return: " << ret << std::endl;
                m_pipeThread->requestStop();


                if (WIFEXITED(status))
                {
                    m_exitcode = WEXITSTATUS(status);
                }
                else
                {
                    // this happens when child was either killed, coredump.
                    // meaning that it is finished, but WIFEXITED does not return true.
                    m_exitcode = -1;
                }

                return Process::ProcessStatus::FINISHED;
            }

        }
        return Process::ProcessStatus::TIMEOUT;
    }

    void ProcessImpl::exec(const std::string &path, const std::vector<std::string> &arguments,
                           const std::vector<Process::EnvironmentPair> &extraEnvironment)
    {

        ArgcAndEnv argcAndEnv(path, arguments, extraEnvironment);

        m_pipe.reset( new PipeHolder());

        pid_t child = fork();
        int ret = 0;
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

                closeFileDescriptors({m_pipe->writeFd()});


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
                ret = execvpe(argcAndEnv.path(), argcAndEnv.argc(), argcAndEnv.envp());
                    // if reaches this point, execv failed
                    // for normal execution, execv never returns.
                _exit(ret);

                break;

            default:
                // parent
                // close the write pipe as parent wants to read what the child writes to stdout, stderr
                m_pipe->closeWrite();
                m_pid = child;


                m_pipeThread.reset( new StdPipeThread(m_pipe->readFd()));
                m_pipeThread->start();
                return;

        }

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

}
}