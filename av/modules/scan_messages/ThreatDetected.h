/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.


******************************************************************************************************/

#pragma once

#include <string>

namespace scan_messages
{
    class ThreatDetected
    {
    public:
        ThreatDetected() = default;

        void setUserID(const std::string& userID);
        void setDetectionTime(const std::string& detectionTime);
        void setThreatType(const std::string& threatType);
        void setScanType(const std::string& scanType);
        void setNotificationStatus(const std::string& notificationStatus);
        void setThreatID(const std::string& threatID);
        void setIdSource(const std::string& idSource);
        void setFileName(const std::string& fileName);
        void setFilePath(const std::string& filePath);
        void setActionCode(const std::string& actionCode);

        [[nodiscard]] std::string serialise() const;

    protected:
        std::string m_userID;
        std::string m_detectionTime;
        // SAV Linux always returns this value it means "virus"
        std::string m_threatType = "1";
        std::string m_scanType;
        std::string m_notificationStatus;
        std::string m_threatID;
        // SAV Linux seems to be using this value all the time
        std::string m_idSource = "NameFilenameFilepathCIMD5";
        std::string m_fileName;
        std::string m_filePath;
        std::string m_actionCode;
    };
}