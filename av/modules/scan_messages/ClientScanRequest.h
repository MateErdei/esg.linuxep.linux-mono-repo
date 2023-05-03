// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ScanType.h"

#include "datatypes/AutoFd.h"
#include "datatypes/ISystemCallWrapper.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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
        using pua_exclusions_t = std::vector<std::string>;

        ClientScanRequest() = default;
        ClientScanRequest(datatypes::ISystemCallWrapperSharedPtr sysCalls, datatypes::AutoFd& fd);

        /*
         * Sender side interface - set the fields, then serialise
         */
        void setPath(const std::string& path) { m_path = path; }

        void setScanInsideArchives(bool scanArchive) { m_scanInsideArchives = scanArchive; }

        void setScanInsideImages(bool scanImage) { m_scanInsideImages = scanImage; }

        void setDetectPUAs(bool detectPUAs) { m_detectPUAs = detectPUAs; }

        void setScanType(E_SCAN_TYPE scanType) { m_scanType = scanType; }

        void setUserID(const std::string& userID) { m_userID = userID; }

        void setPid(const std::int64_t pid) { m_pid = pid; }
        void setExecutablePath(const std::string& path) { m_executablePath = path; }

        void setPuaExclusions(pua_exclusions_t value) { excludedPUAs_ = std::move(value); }

        /*
         * fd is donated in this call
         */
        void setFd(int fd) { m_autoFd.reset(fd); }

        [[nodiscard]] std::string serialise() const;
        [[nodiscard]] std::string getPath() const { return m_path; };
        [[nodiscard]] std::string getUserId() const { return m_userID; };
        [[nodiscard]] int getFd() const { return m_autoFd.fd(); }
        [[nodiscard]] E_SCAN_TYPE getScanType() const { return m_scanType; }
        [[nodiscard]] bool getDetectPUAs() const { return m_detectPUAs; }
        [[nodiscard]] pua_exclusions_t getPuaExclusions() const { return excludedPUAs_; }
        [[nodiscard]] bool isOpenEvent() const { return m_scanType == E_SCAN_TYPE_ON_ACCESS_OPEN; }

    protected:
        std::string m_path;
        std::string m_userID;
        E_SCAN_TYPE m_scanType = E_SCAN_TYPE_UNKNOWN;

        bool m_scanInsideArchives = false;
        bool m_scanInsideImages = false;
        bool m_detectPUAs = true;

        std::string m_executablePath;
        std::int64_t m_pid = -1;
        pua_exclusions_t excludedPUAs_;

        //Not serialised
       datatypes::AutoFd m_autoFd;

       const datatypes::ISystemCallWrapperSharedPtr m_syscalls;

    };

    using ClientScanRequestPtr = std::shared_ptr<ClientScanRequest>;
}

