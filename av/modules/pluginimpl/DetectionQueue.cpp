// Copyright 2022, Sophos Limited.  All rights reserved.

#include "DetectionQueue.h"

namespace Plugin
{
    void DetectionQueue::setMaxSize(uint newMaxSize)
    {
        m_maxSize = newMaxSize;
    }

    void DetectionQueue::push(scan_messages::ThreatDetected task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push(std::move(task));
        m_cond.notify_one();
    }

    std::optional<scan_messages::ThreatDetected> DetectionQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return (!isEmpty() || m_stopRequested.load()); });
        if (m_list.empty())
        {
            return std::nullopt;
        }
        scan_messages::ThreatDetected val = std::move(m_list.front());
        m_list.pop();
        return val;
    }

    bool DetectionQueue::isEmpty()
    {
        return m_list.empty();
    }

    bool DetectionQueue::isFull()
    {
        return m_list.size() >= m_maxSize;
    }

    void DetectionQueue::requestStop()
    {
        m_stopRequested = true;
        m_cond.notify_all();
    }

} // namespace Plugin