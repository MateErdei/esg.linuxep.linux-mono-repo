// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "BatchTimer.h"

#include "Logger.h"

#include <future>

BatchTimer::BatchTimer()
    : m_cancelled(false)
{
}

BatchTimer::~BatchTimer()
{
    Cancel();
}

void BatchTimer::Start()
{
    if (!m_callback)
    {
        throw std::logic_error("BatchTimer callback not configured");
    }

    reset();

    m_future = std::async(std::launch::async, [this]{ run(); });
}

void BatchTimer::Cancel()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cancelled = true;
    lock.unlock();
    m_cv.notify_one();

    if (m_future.valid())
    {
        m_future.get();
    }
}

void BatchTimer::Configure(std::function<void()> callback, std::chrono::milliseconds maxBatchTime)
{
    m_callback = callback;
    m_maxBatchTime = maxBatchTime;
}

void BatchTimer::run()
{
    try
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, m_maxBatchTime, [this] { return m_cancelled; }))
        {
            if (m_callback)
            {
                try
                {
                    m_callback();
                }
                catch (const std::exception& ex)
                {
                    m_callbackError = ex.what();
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        LOGERROR("BatchTimer threw exception: " << ex.what());
        throw;
    }
}

void BatchTimer::reset()
{
    if (m_future.valid())
    {
        m_future.get();
    }

    m_callbackError.clear();

    std::unique_lock<std::mutex> lock(m_mutex);
    m_cancelled = false;
}
