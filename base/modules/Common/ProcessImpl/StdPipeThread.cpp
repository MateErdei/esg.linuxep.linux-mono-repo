/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StdPipeThread.h"

#include <Common/UtilityImpl/StrError.h>

#include <fcntl.h>
#include <unistd.h>

namespace
{
    void setNonBlocking(int fd)
    {
        assert(fd >= 0);
        int ret = ::fcntl(fd, F_SETFL, O_NONBLOCK);
        assert(ret == 0);
        ret = ::fcntl(fd, F_SETFD, O_CLOEXEC);
        assert(ret == 0);
        static_cast<void>(ret); // this is to stop the compiler complaining about unused variables in release builds
    }

    int addFD(fd_set* fds, int fd, int maxfd)
    {
        FD_SET(fd, fds); // NOLINT
        return std::max(maxfd, fd);
    }

    class ScopedTriggerFunctor
    {
        std::function<void(void)>& m_functor;

    public:
        ScopedTriggerFunctor(std::function<void(void)>& functor) : m_functor(functor) {}
        ~ScopedTriggerFunctor() { m_functor(); }
    };

} // namespace

Common::ProcessImpl::StdPipeThread::StdPipeThread(int fileDescriptor, std::function<void(void)> notifyFinished) :
    Common::Threads::AbstractThread(),
    m_fileDescriptor(fileDescriptor),
    m_stdoutStream(std::ios_base::out | std::ios_base::ate),
    m_outputLimit(0),
    m_outputSize(0),
    m_notifyFinished(notifyFinished),
    m_outputTrimmed([](std::string) {})
{
    setNonBlocking(fileDescriptor);
}

Common::ProcessImpl::StdPipeThread::StdPipeThread(int fileDescriptor) : StdPipeThread(fileDescriptor, []() {}) {}

Common::ProcessImpl::StdPipeThread::~StdPipeThread()
{
    // destructor must ensure that the thread is not running anymore or
    // seg fault may occur.
    requestStop();
    join();
}

void Common::ProcessImpl::StdPipeThread::run()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    announceThreadStarted();

    char buffer[101];

    int exitFd = m_notifyPipe.readFd();

    fd_set read_fds;
    int max_fd = 0;
    FD_ZERO(&read_fds);

    max_fd = addFD(&read_fds, exitFd, max_fd);
    max_fd = addFD(&read_fds, m_fileDescriptor, max_fd);
    bool finished = false;

    // Make sure that if this process finishes before this loop starts that one attempt is made to read from the pipe.
    bool readPipeOnce = false;
    ScopedTriggerFunctor scopedTriggerFunctor{ m_notifyFinished };
    while ((!finished && !stopRequested()) || !readPipeOnce)
    {
        readPipeOnce = true;
        fd_set read_temp = read_fds;
        int ret = pselect(max_fd + 1, &read_temp, nullptr, nullptr, nullptr, nullptr);
        if (ret < 0)
        {
            int err = errno;
            if (err == EINTR)
            {
                continue;
            }

            LOGFATAL("Failure in monitor file descriptor: " << UtilityImpl::StrError(err) );
            return;
        }
        if (ret > 0)
        {
            if (FD_ISSET(exitFd, &read_temp)) // NOLINT
            {
                finished = true;
            }
        }

        ssize_t nread = 1;

        while (nread > 0)
        {
            nread = read(m_fileDescriptor, buffer, 100);
            if (nread == 0)
            {
                finished = true;
                break;
            }
            else if (nread < 0)
            {
                int err = errno;
                if (err == EAGAIN
#if EAGAIN != EWOULDBLOCK
                    || err == EWOULDBLOCK
#endif
                )
                {
                    break;
                }
                else
                {
                    LOGERROR("Error reading from pipe: " << err << ": " << Common::UtilityImpl::StrError(err));
                }
                break;
            }
            else
            {
                buffer[nread] = '\0';
                m_stdoutStream << buffer;

                m_outputSize += nread;
                trimStream();
            }
        }
    }
}

void Common::ProcessImpl::StdPipeThread::trimStream()
{
    if (m_outputLimit > 0 && m_outputSize > m_outputLimit * 2)
    {
        std::string currentBuffer = m_stdoutStream.str();
        int pivotPoint = m_outputSize - m_outputLimit;
        m_outputTrimmed(currentBuffer.substr(0, pivotPoint));
        m_stdoutStream.str(currentBuffer.substr(pivotPoint));
        m_outputSize = m_outputLimit;
    }
}

std::string Common::ProcessImpl::StdPipeThread::output()
{
    hasFinished();
    trimStream();
    return m_stdoutStream.str();
}

void Common::ProcessImpl::StdPipeThread::setOutputTrimmedCallBack(std::function<void(std::string)> outputTrimmed)
{
    m_outputTrimmed = outputTrimmed;
}
