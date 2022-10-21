//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ScanType.h"

#include "datatypes/AutoFd.h"

#include <chrono>
#include <memory>
#include <string>

namespace scan_messages
{
    /**
     * Convert Scan type Enum to string.
     * @param scanType
     * @return
     */
    std::string getScanTypeAsStr(const E_SCAN_TYPE& scanType);

    class ClientScanRequest
    {
    public:

        ClientScanRequest() = default;

        /*
         * Sender side interface - set the fields, then serialise
         */
        void setPath(const std::string& path) { m_path = path; }

        void setScanInsideArchives(bool scanArchive) { m_scanInsideArchives = scanArchive; }

        void setScanInsideImages(bool scanImage) { m_scanInsideImages = scanImage; }

        void setScanType(E_SCAN_TYPE scanType) { m_scanType = scanType; }

        void setUserID(const std::string& userID) { m_userID = userID; }

        void setQueueSizeAtTimeOfInsert(const size_t& queueSize) { m_queueSizeAtTimeOfInsert = queueSize; }

        /*
         * fd is donated in this call
         */
        void setFd(int fd) { m_autoFd.reset(fd); }

        [[nodiscard]] std::string serialise() const;
        [[nodiscard]] std::string getPath() const { return m_path; };
        [[nodiscard]] int getFd() const { return m_autoFd.fd(); }
        [[nodiscard]] E_SCAN_TYPE getScanType() const { return m_scanType; }
        [[nodiscard]] std::chrono::steady_clock::time_point getCreationTime() const { return m_creationTime; }
        [[nodiscard]] size_t getQueueSizeAtTimeOfInsert() const { return m_queueSizeAtTimeOfInsert; }
        [[nodiscard]] bool isOpenEvent() const { return m_scanType == E_SCAN_TYPE_ON_ACCESS_OPEN; }

    protected:
        std::string m_path;
        bool m_scanInsideArchives = false;
        bool m_scanInsideImages = false;
        std::string m_userID;
        E_SCAN_TYPE m_scanType = E_SCAN_TYPE_UNKNOWN;

        //Not serialised
       datatypes::AutoFd m_autoFd;
       std::chrono::steady_clock::time_point m_creationTime = std::chrono::steady_clock::now();
       size_t m_queueSizeAtTimeOfInsert;
    };

    using ClientScanRequestPtr = std::shared_ptr<ClientScanRequest>;
}

