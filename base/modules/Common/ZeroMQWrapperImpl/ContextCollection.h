// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

class ContextCollection
{
public:
    static ContextCollection& getInstance()
    {
        static ContextCollection instance;
        return instance;
    }

    ContextCollection(ContextCollection const&) = delete;
    void operator=(ContextCollection const&) = delete;

    void* createContext();
    void closeAll();

    std::mutex m_mutex;
    std::condition_variable m_cond;
private:
    ContextCollection() {}
    std::vector<void*> m_zmqContexts;
};