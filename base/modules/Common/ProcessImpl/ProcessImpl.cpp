//
// Created by pair on 12/06/18.
//

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

namespace
{
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



int p_exec(std::string path,  std::vector<std::string> arguments,
         std::vector<Common::Process::EnvironmentPair> extraEnvironment)
{
    std::vector<char*> argc;
    // const_cast is needed because execvp prototype wants an
    // array of char*, not const char*.
    argc.emplace_back(const_cast<char*>(path.c_str()));
    for (auto const& arg : arguments)
        argc.emplace_back(const_cast<char*>(arg.c_str()));
    // NULL terminate
    argc.push_back(nullptr);


    if (extraEnvironment.size() > 0)
    {
        std::vector<std::string> auxEnv;
        for (auto const& env : extraEnvironment)
        {
            auxEnv.emplace_back(env.first + "=" + env.second);
        }

        std::vector<char*> environmentc;
        // const_cast is needed because execvp prototype wants an
        // array of char*, not const char*.
        for (auto const& env : auxEnv)
        {
            environmentc.emplace_back(const_cast<char *>(env.c_str()));
        }
        // NULL terminate
        environmentc.push_back(nullptr);

        return execvpe(path.c_str(), argc.data(), environmentc.data());
    }

    return execv(path.c_str(), argc.data());

}


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

    class StdPipeThread
    {
    private:
        std::thread m_thread;
        bool m_stopThread;
        int m_fileDescriptor;
        std::stringstream m_stdoutStream;
        std::mutex m_mutex;
        std::mutex m_threadStarted;
        std::condition_variable m_ensureThreadStarted;
    public:
        explicit StdPipeThread(int fileDescriptor)
                : m_thread()
                , m_stopThread(false)
                , m_fileDescriptor(fileDescriptor)
                , m_mutex()
                , m_ensureThreadStarted()
        {

        }
        void setStop( )
        {
            m_stopThread = true;
        }
        ~StdPipeThread()
        {

            m_stopThread = true;
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }
        void start()
        {
            std::unique_lock<std::mutex> lock(m_threadStarted);
            m_thread = std::thread(&StdPipeThread::run,this);
            m_ensureThreadStarted.wait(lock);
        }

        std::string output()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_stdoutStream.str();
        }
        bool hasFinished()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            return true;
        }


    private:
        void run()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            {
                std::unique_lock<std::mutex> templock(m_threadStarted);
                m_ensureThreadStarted.notify_all();
            }
            FileDescriptorHolder input( m_fileDescriptor);
            char buffer[101];
            while (!m_stopThread)
            {
                size_t nread = read(input.fileDescriptor(), buffer, 100);
                if ( nread == 0 )
                {
                    return;
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
        if( m_pipeThread)
        {
            m_pipeThread->setStop();
        }

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

            if( i!= 0)
            {
                std::this_thread::sleep_for(period);
            }

            int status;
            pid_t ret = waitpid(m_pid, &status, WNOHANG);

            if (ret != 0)
            {
                m_pipeThread->setStop();
                if (WIFEXITED(status))
                {
                    m_exitcode = WEXITSTATUS(status);
                }

                return Process::ProcessStatus::FINISHED;
            }

        }
        return Process::ProcessStatus::TIMEOUT;
    }

    void ProcessImpl::exec(const std::string &path, const std::vector<std::string> &arguments,
                           const std::vector<Process::EnvironmentPair> &extraEnvironment)
    {
        for(auto & env : extraEnvironment)
        {
            // make sure all environment variables are valid.
            if(env.first.empty())
            {
                throw Common::Process::IProcessException("Environment name cannot be empty: '' = " + env.second);
            }
        }

        m_pipe.reset( new PipeHolder());

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

                closeFileDescriptors({m_pipe->writeFd()});


                // redirect stdout
                if (dup2(m_pipe->writeFd(), STDOUT_FILENO) == -1)
                {
                    perror("Dup stdout error");
                    exit(errno);
                }

                // redirect stderr
                if (dup2(m_pipe->writeFd(), STDERR_FILENO) == -1)
                {
                    perror("Dup stderr error");
                    exit(errno);
                }
                m_pipe->closeRead();

                {
                    int ret = p_exec( path, arguments, extraEnvironment);
                    // if reaches this point, execv failed
                    // for normal execution, execv never returns.
                    exit(ret);
                }
                break;

            default:
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
                            break;
                    }
                }
                // either call wait, or if m_exit code already populated, return it.
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