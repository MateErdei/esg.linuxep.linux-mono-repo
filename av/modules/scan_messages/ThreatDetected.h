// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ScanType.h"

#include "datatypes/AutoFd.h"

#include <string>

#include <ThreatDetected.capnp.h>

namespace scan_messages
{
    enum E_THREAT_TYPE: int
    {
        // SAV Linux always uses 1, but we might to add more threat types in the future
        E_VIRUS_THREAT_TYPE = 1
    };

    enum E_NOTIFCATION_STATUS: int
    {
        E_NOTIFICATION_STATUS_CLEANED_UP      = 50,
        E_NOTIFICATION_STATUS_NOT_CLEANUPABLE = 200,
        E_NOTIFICATION_STATUS_CLEANUPABLE     = 300,
        E_NOTIFICATION_STATUS_REBOOT_REQUIRED = 500
    };

    enum  E_ACTION_CODE: int
    {
        E_SMT_THREAT_ACTION_UNKNOWN = 100,
        E_SMT_THREAT_ACTION_NONE = 101,
        E_SMT_THREAT_ACTION_RENAME = 102,
        E_SMT_THREAT_ACTION_DELETE = 103,
        E_SMT_THREAT_ACTION_SHRED = 104,
        E_SMT_THREAT_ACTION_MOVE = 105,
        E_SMT_THREAT_ACTION_COPY = 106,
        E_SMT_THREAT_ACTION_ATTACHMENT_DELETED = 107,
        E_SMT_THREAT_ACTION_ATTACHMENT_QUARANTINED = 108,
        E_SMT_THREAT_ACTION_DISINFECTED = 109,
        E_SMT_THREAT_ACTION_MESSAGE_DELETED = 110,
        E_SMT_THREAT_ACTION_MESSAGE_QUARANTINED = 111,
        E_SMT_THREAT_ACTION_AUTHORISED = 112,
        E_SMT_THREAT_ACTION_REMOVED = 113,
        E_SMT_THREAT_ACTION_PARTIALLY_REMOVED = 114,
        E_SMT_THREAT_ACTION_CLEARED_BY_ADMINISTRATOR = 115,
        E_SMT_THREAT_ACTION_BLOCKED = 116
    };

    class ThreatDetected
    {
    public:
        explicit ThreatDetected(
            std::string userID,
            std::int64_t detectionTime,
            E_THREAT_TYPE threatType,
            std::string threatName,
            E_SCAN_TYPE scanType,
            E_NOTIFCATION_STATUS notificationStatus,
            std::string filePath,
            E_ACTION_CODE actionCode,
            std::string sha256,
            std::string threatId,
            datatypes::AutoFd autoFd);

        explicit ThreatDetected(Sophos::ssplav::ThreatDetected::Reader& reader);

        bool operator==(const ThreatDetected& other) const;

        void setUserID(const std::string& userID);
        void setDetectionTime(const std::int64_t& detectionTime);
        void setThreatType(E_THREAT_TYPE threatType);
        void setThreatName(const std::string& threatName);
        void setScanType(E_SCAN_TYPE scanType);
        void setNotificationStatus(E_NOTIFCATION_STATUS notificationStatus);
        void setFilePath(const std::string& filePath);
        void setActionCode(E_ACTION_CODE actionCode);
        void setSha256(const std::string& sha256);
        void setThreatId(const std::string& threatId);
        void setAutoFd(datatypes::AutoFd&& autoFd);

        [[nodiscard]] std::string serialise() const;

        [[nodiscard]] std::string getFilePath() const;
        [[nodiscard]] std::string getThreatName() const;
        [[nodiscard]] bool hasFilePath() const;
        [[nodiscard]] std::int64_t getDetectionTime() const;
        [[nodiscard]] std::string getUserID() const;
        [[nodiscard]] E_SCAN_TYPE getScanType() const;
        [[nodiscard]] E_NOTIFCATION_STATUS getNotificationStatus() const;
        [[nodiscard]] E_THREAT_TYPE getThreatType() const;
        [[nodiscard]] E_ACTION_CODE getActionCode() const;
        [[nodiscard]] std::string getSha256() const;
        [[nodiscard]] std::string getThreatId() const;

        [[nodiscard]] int getFd() const; // does not transfer ownership of fd, use moveAutoFd if that's needed
        [[nodiscard]] datatypes::AutoFd moveAutoFd();

    protected:
        std::string m_userID;
        std::int64_t m_detectionTime;
        E_THREAT_TYPE m_threatType;
        std::string m_threatName;
        E_SCAN_TYPE m_scanType;
        E_NOTIFCATION_STATUS m_notificationStatus;
        std::string m_filePath;
        E_ACTION_CODE m_actionCode;
        std::string m_sha256;
        std::string m_threatId;

        // Not serialised, sent over socket using send_fd
        datatypes::AutoFd m_autoFd;
    };
} // namespace scan_messages