// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ServerThreatDetected.h"

using namespace scan_messages;

ServerThreatDetected::ServerThreatDetected(Sophos::ssplav::ThreatDetected::Reader& reader) :
    ThreatDetected(
        reader.getUserID(),
        reader.getDetectionTime(),
        static_cast<E_THREAT_TYPE>(reader.getThreatType()),
        reader.getThreatName(),
        static_cast<E_SCAN_TYPE>(reader.getScanType()),
        static_cast<E_NOTIFCATION_STATUS>(reader.getNotificationStatus()),
        reader.getFilePath(),
        static_cast<E_ACTION_CODE>(reader.getActionCode()),
        reader.getSha256(),
        reader.getThreatId(),
        datatypes::AutoFd())
{
}

std::string ServerThreatDetected::getFilePath() const
{
    return m_filePath;
}

std::string ServerThreatDetected::getThreatName() const
{
    return m_threatName;
}

bool ServerThreatDetected::hasFilePath() const
{
    return !m_filePath.empty();
}

std::int64_t ServerThreatDetected::getDetectionTime() const
{
    return m_detectionTime;
}

std::string ServerThreatDetected::getUserID() const
{
    return m_userID;
}

E_SCAN_TYPE ServerThreatDetected::getScanType() const
{
    return m_scanType;
}

E_NOTIFCATION_STATUS ServerThreatDetected::getNotificationStatus() const
{
    return m_notificationStatus;
}

E_THREAT_TYPE ServerThreatDetected::getThreatType() const
{
    return m_threatType;
}

E_ACTION_CODE ServerThreatDetected::getActionCode() const
{
    return m_actionCode;
}

std::string ServerThreatDetected::getSha256() const
{
    return m_sha256;
}

std::string ServerThreatDetected::getThreatId() const
{
    return m_threatId;
}