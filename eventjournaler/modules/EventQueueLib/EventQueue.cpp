/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <optional>
#include "EventQueue.h"
#include "Logger.h"

namespace EventQueueLib
{
    EventQueueLib::EventQueue::EventQueue(int maxSize) :
        m_maxQueueLength(maxSize)
    {
    }

    bool EventQueueLib::EventQueue::isQueueFull()
    {
        return m_queue.size() >= m_maxQueueLength;
    }

    bool EventQueueLib::EventQueue::isQueueEmpty()
    {
        return m_queue.empty();
    }

    bool EventQueueLib::EventQueue::push(JournalerCommon::Event event)
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (isQueueFull())
        {
            return false;
        }
        m_queue.push(std::move(event));
        m_cond.notify_one();
        LOGDEBUG("Queue size after push: " << m_queue.size());

        return true;
    }

    std::optional<JournalerCommon::Event> EventQueueLib::EventQueue::pop(int timeoutInMilliseconds)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        bool queueEmpty = !m_cond.wait_for(lock, std::chrono::milliseconds(timeoutInMilliseconds), [this] { return !isQueueEmpty(); });

        if (queueEmpty)
        {
            return std::nullopt;
        }
        auto value = m_queue.front();
        m_queue.pop();
        return value;
    }

}

