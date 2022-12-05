// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "ScanType.h"

#include "datatypes/AutoFd.h"

#include "common/CentralEnums.h"

#include <ctime>
#include <string>

#include <ThreatDetected.capnp.h>

namespace scan_messages
{
    // TODO LINUXDAR-5839: E_NOTIFCATION_STATUS and E_ACTION_CODE should be combined into a single enum to correspond
    // with event-core-clean fields

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

    struct ThreatDetected
    {
    public:
        ThreatDetected(
            std::string userID,
            std::int64_t detectionTime,
            common::CentralEnums::ThreatType threatType,
            std::string threatName,
            E_SCAN_TYPE scanType,
            E_NOTIFCATION_STATUS notificationStatus,
            std::string filePath,
            E_ACTION_CODE actionCode,
            std::string sha256,
            std::string threatId,
            bool isRemote,
            common::CentralEnums::ReportSource reportSource,
            datatypes::AutoFd autoFd);

        explicit ThreatDetected(Sophos::ssplav::ThreatDetected::Reader& reader);

        bool operator==(const ThreatDetected& other) const;

        [[nodiscard]] std::string serialise() const;

        // Checks whether the contained data is valid; throws if not
        void validate() const;

        std::string userID;
        std::int64_t detectionTime;
        common::CentralEnums::ThreatType threatType;
        std::string threatName;
        E_SCAN_TYPE scanType;
        E_NOTIFCATION_STATUS notificationStatus;
        std::string filePath;
        E_ACTION_CODE actionCode;
        std::string sha256;
        std::string threatId;
        bool isRemote;
        common::CentralEnums::ReportSource reportSource;

        std::int64_t pid =-1;
        std::string executablePath;

        // Not serialised, sent over socket using send_fd
        datatypes::AutoFd autoFd;
    };
} // namespace scan_messages