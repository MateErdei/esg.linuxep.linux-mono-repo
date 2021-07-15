/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <optional>
#include "EventQueue.h"

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

    bool EventQueueLib::EventQueue::push(Common::ZeroMQWrapper::data_t event)
    {
        std::lock_guard<std::mutex> lock(m_pushMutex);
        if (isQueueFull())
        {
            return false;
        }
        m_queue.push(event);
        m_cond.notify_one();
        return true;
    }

    std::optional<Common::ZeroMQWrapper::data_t> EventQueueLib::EventQueue::pop(int timeoutInMilliseconds)
    {
        std::unique_lock<std::mutex> lock(m_popMutex);
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

