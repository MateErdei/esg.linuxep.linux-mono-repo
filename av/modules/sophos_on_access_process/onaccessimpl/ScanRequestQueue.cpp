//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestQueue.h"

#include "Logger.h"

using namespace sophos_on_access_process::onaccessimpl;
using namespace std::chrono_literals;

ScanRequestQueue::ScanRequestQueue(size_t maxSize)
    : m_maxSize(maxSize)
{

}

ScanRequestQueue::~ScanRequestQueue()
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
        m_queue.emplace(item);
        m_condition.notify_one();
        return true;
    }
}

 ClientScanRequestPtr ScanRequestQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_lock);
    // release lock as long as the wait and re-aquire it afterwards.
    if (m_condition.wait_for(lock, 100ms, [this]{ return !m_queue.empty(); }))
    {
        auto request = m_queue.front();
        m_queue.pop();
        return request;
    }
    else
    {
        return nullptr;
    }
}

size_t ScanRequestQueue::size() const
{
    return m_queue.size();
}
