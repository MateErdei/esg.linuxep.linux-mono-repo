/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <atomic>
#include <csignal>
#include <functional>
#include <iostream>
#include <pthread.h>
#include <thread>

//Todo extend namespace to entire contents of file
namespace fuzzer
{
    static const std::string testdir = "/opt/test/inputs";
    static const std::string targetdir = testdir + "/fuzz_targets";


    enum class ThreadStatus
    {
        NOTSTARTED,
        RUNNING,
        FINISHED
    };
    
    class ThreadGuard
    {
        std::atomic<ThreadStatus>& m_thread_status;

    public:
        ThreadGuard(std::atomic<ThreadStatus>& thread_status) : m_thread_status(thread_status)
        {
            m_thread_status = ThreadStatus::RUNNING;
        }
        ~ThreadGuard() { m_thread_status = ThreadStatus::FINISHED; }
    };

    class Runner
    {
    public:
        Runner();
        void setMainLoop(std::function<void()> mainLoop);

        bool threadRunning() const;

        virtual ~Runner();

    private:
        std::atomic<ThreadStatus> m_thread_status;
        std::thread m_thread;
    };
}