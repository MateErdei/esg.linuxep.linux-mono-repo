// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ClientScanRequest.h"

#include <chrono>

namespace sophos_on_access_process::onaccessimpl
{
    class OnAccessScanRequest : public scan_messages::ClientScanRequest
    {
    public:
        using scan_messages::ClientScanRequest::ClientScanRequest;
        [[nodiscard]] std::chrono::steady_clock::time_point getCreationTime() const { return m_creationTime; }

        [[nodiscard]] size_t getQueueSizeAtTimeOfInsert() const { return m_queueSizeAtTimeOfInsert; }
        void setQueueSizeAtTimeOfInsert(const size_t& queueSize) { m_queueSizeAtTimeOfInsert = queueSize; }

        using unique_t = std::pair<dev_t, ino_t>;
        [[nodiscard]] unique_t uniqueMarker() const;

        using hash_t = std::size_t;
        [[nodiscard]] std::optional<hash_t> hash() const;

        bool operator==(const OnAccessScanRequest& other) const;

        bool isCached() const
        {
            return m_isCached;
        }

        void setIsCached(bool value)
        {
            m_isCached = value;
        }

    protected:
        mutable struct stat m_fstat{};

        /**
        *
        * @return true if we were able to fstat the file
         */
        bool fstatIfRequired() const;

    private:
        const std::chrono::steady_clock::time_point m_creationTime = std::chrono::steady_clock::now();
        size_t m_queueSizeAtTimeOfInsert = 0;

        bool m_isCached = false;
    };
}