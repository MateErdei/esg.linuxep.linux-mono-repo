//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ScanType.h"

#include "datatypes/AutoFd.h"

#include <memory>
#include <string>

namespace scan_messages
{
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

        /*
         * fd is donated in this call
         */
        void setFd(int fd) { m_autoFd.reset(fd); }

        [[nodiscard]] std::string serialise() const;
        [[nodiscard]] std::string getPath() const { return m_path; };
        [[nodiscard]] int getFd() const { return m_autoFd.fd(); }

    protected:
        std::string m_path;
        bool m_scanInsideArchives = false;
        bool m_scanInsideImages = false;
        std::string m_userID;
        E_SCAN_TYPE m_scanType = E_SCAN_TYPE_UNKNOWN;

        //Not serialised
       datatypes::AutoFd m_autoFd;
    };

    using ClientScanRequestPtr = std::shared_ptr<ClientScanRequest>;
}

