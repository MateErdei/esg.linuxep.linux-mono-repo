/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Threads/NotifyPipe.h"

#include <deque>
#include <mutex>
#include <optional>

namespace lockedqueue
{
    template<class T>
    class LockedQueue
    {
    public:
        void add(T item);
        std::optional<T> get();
        int notifyFd();
    private:
        std::mutex m_lock;
        std::deque<T> m_queue;
        Common::Threads::NotifyPipe m_notifyPipe;

    };

    template<class T>
    void LockedQueue<T>::add(T item)
    {
        std::scoped_lock<std::mutex> lock(m_lock);
        m_queue.push_back(std::move(item));
        m_notifyPipe.notify();
    }

    template<class T>
    std::optional<T> LockedQueue<T>::get()
    {
        std::scoped_lock<std::mutex> lock(m_lock);
        if (m_notifyPipe.notified())
        {
            return m_queue.pop_front();
        }
        return std::optional<T>();
    }

    template<class T>
    int LockedQueue<T>::notifyFd()
    {
        return m_notifyPipe.readFd();
    }
}
