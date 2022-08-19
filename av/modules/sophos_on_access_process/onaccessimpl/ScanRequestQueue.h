//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "scan_messages/ClientScanRequest.h"

#include <queue>
#include <mutex>
#include <condition_variable>

using namespace scan_messages;

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestQueue
    {
    public:
        ScanRequestQueue();
        ~ScanRequestQueue();

        void push(std::shared_ptr<ClientScanRequest> scanRequest);
        std::shared_ptr<ClientScanRequest> pop();

        size_t size();

    private:
        std::queue<std::shared_ptr<ClientScanRequest>> m_queue;
        mutable std::mutex m_lock;
        std::condition_variable m_condition;
    };
}