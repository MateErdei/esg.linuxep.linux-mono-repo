//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestQueue.h"

#include "Logger.h"

using namespace sophos_on_access_process::onaccessimpl;
using namespace std::chrono_literals;

ScanRequestQueue::ScanRequestQueue(size_t maxSize)
    : m_maxSize(maxSize)
{
}

bool ScanRequestQueue::emplace(ClientScanRequestPtr item)
{
    size_t currentQueueSize = size();
    if (currentQueueSize >= m_maxSize)
    {
        LOGWARN("Unable to add scan request to queue as it is at full capacity: " << currentQueueSize);
        return false;
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_lock);
        item->setQueueIndex(currentQueueSize);
        m_queue.emplace(std::move(item));
        m_condition.notify_one();
        return true;
    }
}

ClientScanRequestPtr ScanRequestQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_lock);
    // release lock as long as the wait and re-acquire it afterwards.
    m_condition.wait(lock, [this]{ return m_shuttingDown.load() || !m_queue.empty(); });
    ClientScanRequestPtr scanRequest;
    if (!m_shuttingDown.load())
    {
        scanRequest = m_queue.front();
        m_queue.pop();
        assert(scanRequest);
    }
    assert(m_shuttingDown.load() || scanRequest);

    return scanRequest;
}

void ScanRequestQueue::stop()
{
    m_shuttingDown.store(true);

    std::unique_lock<std::mutex> lock(m_lock);
    typeof(m_queue) empty;
    m_queue.swap(empty);

    m_condition.notify_all();
}

size_t ScanRequestQueue::size() const
{
    return m_queue.size();
}

void ScanRequestQueue::restart()
{
    std::unique_lock<std::mutex> lock(m_lock);
    if (!m_queue.empty())
    {
        typeof(m_queue) empty;
        m_queue.swap(empty);
    }

    m_shuttingDown.store(false);
}
