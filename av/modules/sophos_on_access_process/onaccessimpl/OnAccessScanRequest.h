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

    private:
        const std::chrono::steady_clock::time_point m_creationTime = std::chrono::steady_clock::now();
        size_t m_queueSizeAtTimeOfInsert = 0;
    };
}
