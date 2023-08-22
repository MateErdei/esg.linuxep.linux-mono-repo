// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "SignalHandler.h"

#include <csignal>

namespace
{
    Common::Threads::NotifyPipe GL_CHILD_PROCESS_TERMINATED_PIPE;
    Common::Threads::NotifyPipe GL_TERM_PIPE;
    Common::Threads::NotifyPipe GL_USR1_PIPE;

    void signal_handler(int signal)
    {
        if (signal == SIGCHLD)
        {
            GL_CHILD_PROCESS_TERMINATED_PIPE.notify();
        }
        else if (signal == SIGINT || signal == SIGTERM)
        {
            GL_TERM_PIPE.notify();
        }
        else if (signal == SIGUSR1)
        {
            GL_USR1_PIPE.notify();
        }
    }

    void setSignalHandler()
    {
        struct sigaction signalBuf;
        signalBuf.sa_handler = signal_handler;
        sigemptyset(&signalBuf.sa_mask);
        signalBuf.sa_flags = SA_NOCLDSTOP | SA_RESTART;
        ::sigaction(SIGCHLD, &signalBuf, nullptr);
        ::sigaction(SIGINT, &signalBuf, nullptr);
        ::sigaction(SIGTERM, &signalBuf, nullptr);
        ::sigaction(SIGUSR1, &signalBuf, nullptr);
    }

    void clearSignalHandler()
    {
        struct sigaction signalBuf;
        signalBuf.sa_handler = SIG_DFL;
        sigemptyset(&signalBuf.sa_mask);
        signalBuf.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_NOCLDWAIT;
        ::sigaction(SIGCHLD, &signalBuf, nullptr);
        ::sigaction(SIGINT, &signalBuf, nullptr);
        ::sigaction(SIGTERM, &signalBuf, nullptr);
        ::sigaction(SIGUSR1, &signalBuf, nullptr);
    }

} // namespace

namespace Common::ProcessMonitoringImpl
{
    SignalHandler::SignalHandler() { setSignalHandler(); }

    SignalHandler::~SignalHandler() { clearSignalHandler(); }

    bool SignalHandler::clearSubProcessExitPipe()
    {
        bool ret = false;
        while (GL_CHILD_PROCESS_TERMINATED_PIPE.notified())
        {
            ret = true;
        }
        return ret;
    }

    bool SignalHandler::clearTerminationPipe()
    {
        bool ret = false;
        while (GL_TERM_PIPE.notified())
        {
            ret = true;
        }
        return ret;
    }

    bool SignalHandler::clearUSR1Pipe()
    {
        bool ret = false;
        while (GL_USR1_PIPE.notified())
        {
            ret = true;
        }
        return ret;
    }


    int SignalHandler::subprocessExitFileDescriptor() { return GL_CHILD_PROCESS_TERMINATED_PIPE.readFd(); }

    int SignalHandler::terminationFileDescriptor() { return GL_TERM_PIPE.readFd(); }

    int SignalHandler::usr1FileDescriptor() { return GL_USR1_PIPE.readFd(); }
} // namespace Common::ProcessMonitoringImpl
