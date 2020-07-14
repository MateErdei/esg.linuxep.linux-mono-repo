/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <mutex>
#include <condition_variable>
#include <list>
#include <string>
#include <optional>
#include <chrono>
namespace CommsComponent
{
    class ChannelClosedException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class MessageChannel
    {
        std::mutex m_mutex;
        std::condition_variable m_cond;
        std::list<std::optional<std::string>> m_messages;
        bool m_channelClosedFlag = false;

    public:
        void push(std::string);

        bool pop(std::string &, std::chrono::milliseconds);

        void pop(std::string &);

        void pushStop();

    };
}