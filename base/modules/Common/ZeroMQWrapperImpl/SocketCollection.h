// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

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
    void closeAll();

    std::mutex m_mutex;
    std::condition_variable m_socketCollectionCondition;
private:
    SocketCollection() {}
    std::vector<void*> m_zmqSockets;
};
