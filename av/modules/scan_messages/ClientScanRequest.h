//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ScanType.h"

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
        ClientScanRequest(datatypes::ISystemCallWrapperSharedPtr sysCalls, int fd);

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

        using hash_t = std::size_t;
        std::optional<hash_t> hash() const;
        bool operator==(const ClientScanRequest& other) const;

    protected:
        std::string m_path;
        std::string m_userID;
        E_SCAN_TYPE m_scanType = E_SCAN_TYPE_UNKNOWN;

        bool m_scanInsideArchives = false;
        bool m_scanInsideImages = false;

        //Not serialised
       datatypes::AutoFd m_autoFd;
       const std::chrono::steady_clock::time_point m_creationTime = std::chrono::steady_clock::now();
       size_t m_queueSizeAtTimeOfInsert = 0;
   private:
       /**
        *
        * @return true if we were able to fstat the file
        */
       bool fstatIfRequired() const;
       using key_t = std::pair<dev_t, ino_t>;

       mutable struct stat m_fstat{};

       const datatypes::ISystemCallWrapperSharedPtr m_syscalls;

    };

    using ClientScanRequestPtr = std::shared_ptr<ClientScanRequest>;
}

