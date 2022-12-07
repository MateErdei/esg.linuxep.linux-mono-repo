//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestQueue.h"

#include "Logger.h"

using namespace sophos_on_access_process::onaccessimpl;
using namespace std::chrono_literals;

ScanRequestQueue::ScanRequestQueue(size_t maxSize, bool useDeDup)
    : m_maxSize(maxSize), m_useDeDup(useDeDup)
{
}

bool ScanRequestQueue::emplace(ClientScanRequestPtr item)
{
    std::lock_guard<std::mutex> lock(m_lock);
    size_t currentQueueSize = m_queue.size();
    if (currentQueueSize >= m_maxSize)
    {
        LOGWARN("Unable to add scan request to queue as it is at full capacity: " << currentQueueSize);
        return false;
    }
    else
    {
        if (m_useDeDup)
        {
            const auto hashOptional = item->hash();
            if (hashOptional.has_value())
            {
                const auto& value = item->uniqueMarker();
                const auto hash = hashOptional.value();
                const auto& previousItem = m_deDupData.find(hash);
                if (previousItem != m_deDupData.end())
                {
                    if (previousItem->second == value)
                    {
                        LOGTRACE(
                            "Skipping scan of " << item->getPath() << " fd=" << item->getFd() << " ("
                                                << (item->isOpenEvent() ? "Open" : "Close-Write") << ')');
                        return true; // item has been dealt with
                    }
                    else
                    {
                        // hash collision
                        LOGTRACE("Hash collision in dedup for " << item->getPath());
                    }
                }
                // else record it in the set
                m_deDupData[hash] = value;
            }
            // Else failed to fstat the file
        }
        item->setQueueSizeAtTimeOfInsert(currentQueueSize);
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
        if (m_useDeDup)
        {
            const auto hashOptional = scanRequest->hash();
            if (hashOptional.has_value())
            {
                const auto hash = hashOptional.value();
                m_deDupData.erase(hash);
            }
        }
    }
    assert(m_shuttingDown.load() || scanRequest);

    return scanRequest;
}

void ScanRequestQueue::stop()
{
    m_shuttingDown.store(true);
    clearQueue();
    m_condition.notify_all();
}

size_t ScanRequestQueue::size() const
{
    std::lock_guard<std::mutex> lock(m_lock);
    return m_queue.size();
}

void ScanRequestQueue::restart()
{
    clearQueue();
    m_shuttingDown.store(false);
}

void ScanRequestQueue::clearQueue()
{
    std::unique_lock<std::mutex> lock(m_lock);
    typeof(m_queue) empty;
    m_queue.swap(empty);
    m_deDupData.clear();
}
