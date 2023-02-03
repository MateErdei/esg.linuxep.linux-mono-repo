// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <condition_variable>
#include <mutex>
#include <set>

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
    void closeContext(void* context);
    void closeAll();

    std::mutex m_mutex;
private:
    ContextCollection() {}
    std::set<void*> m_zmqContexts;
};