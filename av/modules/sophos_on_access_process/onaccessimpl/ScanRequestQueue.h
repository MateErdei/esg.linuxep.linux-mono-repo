// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "OnAccessScanRequest.h"

#include "datatypes/AutoFd.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <utility>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

using namespace scan_messages;

namespace sophos_on_access_process::onaccessimpl
{
    class ScanRequestQueue
    {
    public:
        using scan_request_t = OnAccessScanRequest;
        using scan_request_ptr_t = std::shared_ptr<scan_request_t>;
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
        bool emplace(scan_request_ptr_t item);

        /**
         * Returns pair containing the first scan request and associated file descriptor in the queue (FIFO)
         * Waits to acquire m_lock before attempting to modify the queue
         */
        scan_request_ptr_t pop();

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

    TEST_PUBLIC:
        using dedup_map_t = std::unordered_map<ClientScanRequest::hash_t, ClientScanRequest::unique_t>;
        dedup_map_t m_deDupData;

    private:
        void clearQueue();

        std::queue<scan_request_ptr_t> m_queue;

        mutable std::mutex m_lock;
        std::condition_variable m_condition;

        const size_t m_maxSize;
        std::atomic_bool m_shuttingDown = false;
        bool m_useDeDup;
    };

    using ScanRequestQueueSharedPtr = std::shared_ptr<ScanRequestQueue>;
}