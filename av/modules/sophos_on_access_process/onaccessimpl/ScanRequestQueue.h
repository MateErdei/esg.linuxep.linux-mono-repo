// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "scan_messages/ClientScanRequest.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <utility>

using namespace scan_messages;

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestQueue
    {
    public:
        /**
         *
         * @param maxSize
         * @param useDeDup Perform de-dup on incoming requests
         */
        explicit ScanRequestQueue(size_t maxSize, bool useDeDup=true);

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
         * Resets the state so that we can continue using the queue
         */
        void restart();

        /**
         * Returns the current size of m_queue
         */
        size_t size() const;

    private:
        void clearQueue();


        std::queue<ClientScanRequestPtr> m_queue;

        mutable std::mutex m_lock;
        std::condition_variable m_condition;

        const size_t m_maxSize;
        std::atomic_bool m_shuttingDown = false;
        bool m_useDeDup;
        using dedup_set_t = std::unordered_set<ClientScanRequest::hash_t>;
        dedup_set_t m_deDupSet;
    };

    using ScanRequestQueueSharedPtr = std::shared_ptr<ScanRequestQueue>;
}