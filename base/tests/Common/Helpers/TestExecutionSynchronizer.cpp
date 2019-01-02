/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "TestExecutionSynchronizer.h"
namespace Tests
{

    TestExecutionSynchronizer::TestExecutionSynchronizer(int toBeNotifiedNTimes)
            : m_calledNTimes(0)
            , m_expectedCall(toBeNotifiedNTimes)
    {

    }

    void TestExecutionSynchronizer::notify()
    {
        m_calledNTimes++;
        if (m_calledNTimes >= m_expectedCall)
        {
            m_promise.set_value();
        }
    }

    bool TestExecutionSynchronizer::waitfor(int ms)
    {
        return waitfor(std::chrono::milliseconds(ms));
    }
    bool TestExecutionSynchronizer::waitfor(std::chrono::milliseconds ms)
    {
        // Effective Modern C++
        // Item39: Consider void futures for one-shot event communication
        return m_promise.get_future().wait_for(ms) == std::future_status::ready;

    }



    ReentrantExecutionSynchronizer::ReentrantExecutionSynchronizer(): m_counter(0), m_FirstMutex(), m_waitsync()
    {

    }

    void ReentrantExecutionSynchronizer::notify()
    {
        std::lock_guard<std::mutex> lock( m_FirstMutex);
        m_counter++;
        m_waitsync.notify_all();
    }

    void ReentrantExecutionSynchronizer::notify(int expectedCounter)
    {
        std::lock_guard<std::mutex> lock( m_FirstMutex);
        m_counter++;
        if (m_counter != expectedCounter)
        {
            throw std::runtime_error("notify with bad counter");
        }
        m_waitsync.notify_all();
    }

    void ReentrantExecutionSynchronizer::waitfor(int expectedCounter, int ms)
    {
        waitfor(expectedCounter, std::chrono::milliseconds(ms));
    }

    void ReentrantExecutionSynchronizer::waitfor(int expectedCounter, std::chrono::milliseconds ms)
    {
        std::unique_lock<std::mutex> lock( m_FirstMutex);
        if( m_counter == expectedCounter)
        {
            return;
        }

        while( true )
        {
            std::cv_status cv_status = m_waitsync.wait_for(lock, ms);
            if( cv_status == std::cv_status::timeout)
            {
                throw std::runtime_error("SyncTimeout");
            }
            if( expectedCounter == m_counter)
            {
                return;
            }
        }
    }


}