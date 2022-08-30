//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "scan_messages/ClientScanRequest.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

using namespace scan_messages;

const size_t MAX_SIZE = 1000;

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestQueue
    {
    public:
        ScanRequestQueue(size_t maxSize = MAX_SIZE);
        ~ScanRequestQueue();

        /**
         * Add scan request and associated file descriptor to the queue ready for scanning
         * Returns true on success and false when queue already contains m_maxSize items
         */
        bool emplace(ClientScanRequestPtr item);

        /**
         * Returns pair containing the first scan request and associated file descriptor in the queue (FIFO)
         * Waits to acquire m_lock before attempting to modify the queue
         */
        ClientScanRequestPtr pop();

        /**
         * Releases threads which are waiting in the pop() method so that they can be terminated
         */
        void stop();

        /**
         * Returns the current size of m_queue
         */
        size_t size() const;

    private:
        std::queue<ClientScanRequestPtr> m_queue;
        mutable std::mutex m_lock;
        std::condition_variable m_condition;

        size_t m_maxSize;
        bool m_shuttingDown = false;
    };
}