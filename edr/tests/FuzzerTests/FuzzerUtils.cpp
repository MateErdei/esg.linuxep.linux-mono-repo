/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FuzzerUtils.h"

Runner::Runner() : m_thread_status{ ThreadStatus::NOTSTARTED } {}

void Runner::setMainLoop(std::function<void()> mainLoop)
{
    m_thread = std::thread{ [&, mainLoop]() {
        ThreadGuard threadGuard(m_thread_status);
        mainLoop();
    } };
}

bool Runner::threadRunning() const
{
    // help method to allow tests to verify that ManagementAgent has not crashed.
    return m_thread_status != ThreadStatus::FINISHED;
}

Runner::~Runner()
{
    if (m_thread.joinable())
    {
        std::cout << "Force thread to stop" << std::endl;
        auto m_thread_id = m_thread.native_handle();
        pthread_kill(m_thread_id, SIGINT);
        m_thread.join();
    }
}
