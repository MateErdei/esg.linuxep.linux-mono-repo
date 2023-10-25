// Copyright 2022, Sophos Limited.  All rights reserved.

#include "DetectionQueue.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

namespace Plugin
{
    void DetectionQueue::setMaxSize(uint newMaxSize)
    {
        m_maxSize = newMaxSize;
    }

    bool DetectionQueue::push(scan_messages::ThreatDetected& task)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        if (!isFullLocked(lck))
        {
            m_list.push(std::move(task));
            m_cond.notify_one();
            return true;
        }
        Common::Telemetry::TelemetryHelper::getInstance().increment("detections-dropped-from-safestore-queue", 1ul);
        return false;
    }

    std::optional<scan_messages::ThreatDetected> DetectionQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [&lck, this] { return (!isEmptyLocked(lck) || m_stopRequested.load()); });
        if (isEmptyLocked(lck))
        {
            return std::nullopt;
        }
        scan_messages::ThreatDetected val = std::move(m_list.front());
        m_list.pop();
        return val;
    }

    bool DetectionQueue::isEmpty()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        return isEmptyLocked(lck);
    }

    bool DetectionQueue::isEmptyLocked(std::unique_lock<std::mutex> &)
    {
        return m_list.empty();
    }

    bool DetectionQueue::isFull()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        return isFullLocked(lck);
    }

    bool DetectionQueue::isFullLocked(std::unique_lock<std::mutex> &)
    {
        return m_list.size() >= m_maxSize;
    }

    void DetectionQueue::requestStop()
    {
        m_stopRequested.store(true);
        m_cond.notify_all();
    }


} // namespace Plugin