// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <condition_variable>
#include <mutex>
#include <set>

class SocketCollection
{
public:
    static SocketCollection& getInstance()
    {
        static SocketCollection instance;
        return instance;
    }

    SocketCollection(SocketCollection const&) = delete;
    void operator=(SocketCollection const&) = delete;

    void* createSocket(void* context,
                       const int type);
    void closeSocket(void* socket);
    void closeAll();

    std::mutex m_mutex;
private:
    SocketCollection() {}
    std::set<void*> m_zmqSockets;
};