//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestQueue.h"

#include "Logger.h"

using namespace sophos_on_access_process::onaccessimpl;

ScanRequestQueue::ScanRequestQueue(size_t maxSize)
    : m_maxSize(maxSize)
{

}

ScanRequestQueue::~ScanRequestQueue()
{

}

void ScanRequestQueue::push(std::shared_ptr<ClientScanRequest> scanRequest)
{
    size_t currentQueueSize = size();
    if (currentQueueSize >= m_maxSize)
    {
        LOGWARN("Unable to add scan request to queue as it is at full capacity: " << currentQueueSize);
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_queue.push(scanRequest);
        m_condition.notify_one();
    }
}

std::shared_ptr<ClientScanRequest> ScanRequestQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_lock);
    while(m_queue.empty())
    {
        // release lock as long as the wait and re-aquire it afterwards.
        m_condition.wait(lock);
    }
    std::shared_ptr<ClientScanRequest> scanRequest = m_queue.front();
    m_queue.pop();
    return scanRequest;
}

size_t ScanRequestQueue::size()
{
    return m_queue.size();
}
