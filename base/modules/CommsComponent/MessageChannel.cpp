/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MessageChannel.h"
#include "Logger.h"

namespace CommsComponent {

    void MessageChannel::push(std::string message)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        if (!m_channelClosedFlag)    //if channel closed reject
        {
            m_messages.emplace_back(std::move(message));
            m_cond.notify_one();
        }
    }
    bool MessageChannel::pop(std::string& message , std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        if (!m_channelClosedFlag)    // if channel closed reject
        {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

            if (timeout.count() == -1)
            {
                m_cond.wait(lck, [this] { return !m_messages.empty(); });
            }
            else
            {
                m_cond.wait_until(lck, now + timeout, [this] { return !m_messages.empty(); });
            }

            if (m_messages.empty())
            {
                return false;
            }

            auto optmessage = m_messages.front();
            m_messages.pop_front();
            if (optmessage.has_value())
            {
                message = optmessage.value();
            }
            else
            {
                // set channel is closed flag.
                m_channelClosedFlag = true;
                LOGDEBUG("Message Channel STOP received");
                throw ChannelClosedException("closed");
            }
        }
        else
        {
            LOGDEBUG("Message Channel already had a STOP");
            throw ChannelClosedException("closed");
        }
        return true;

    }
    void MessageChannel::pop(std::string& message)
    {
        pop(message, std::chrono::milliseconds(-1)); 
    }

    void MessageChannel::pushStop()
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        if (!m_channelClosedFlag)    // ignore if already closed
        {
            m_messages.emplace_back(std::nullopt);
            m_cond.notify_one();
        }
    }
}