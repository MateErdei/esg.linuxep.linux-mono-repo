/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatDetected.h"

#include "ThreatDetected.capnp.h"
#include "Logger.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;


ThreatDetected::ThreatDetected()
        :
          m_detectionTime(0),
          m_threatType(E_VIRUS_THREAT_TYPE),
          m_scanType(E_SCAN_TYPE_UNKNOWN),
          m_notificationStatus(E_NOTIFICATION_STATUS_NOT_CLEANUPABLE),
          m_actionCode(E_SMT_THREAT_ACTION_UNKNOWN)
{
}

void ThreatDetected::setUserID(const std::string& userID)
{
    m_userID = userID;
}

void ThreatDetected::setDetectionTime(const std::int64_t & detectionTime)
{
    m_detectionTime = detectionTime;
}

void ThreatDetected::setThreatName(const std::string &threatName)
{
    m_threatName = threatName;
}

void ThreatDetected::setThreatType(E_THREAT_TYPE threatType)
{
    m_threatType = threatType;
}

void ThreatDetected::setScanType(E_SCAN_TYPE scanType)
{
    m_scanType = scanType;
}

void ThreatDetected::setNotificationStatus(E_NOTIFCATION_STATUS notificationStatus)
{
    m_notificationStatus = notificationStatus;
}

void ThreatDetected::setFilePath(const std::string& filePath)
{
    if (filePath.empty())
    {
        // TODO: Should this be a warning?
        LOGERROR("Missing    path in threat report: empty string");
    }

    m_filePath = filePath;
}

void ThreatDetected::setActionCode(E_ACTION_CODE actionCode)
{
    m_actionCode = actionCode;
}

std::string ThreatDetected::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::ThreatDetected::Builder threatDetectedBuilder =
            message.initRoot<Sophos::ssplav::ThreatDetected>();

    threatDetectedBuilder.setUserID(m_userID);
    threatDetectedBuilder.setDetectionTime(m_detectionTime);
    threatDetectedBuilder.setThreatType(m_threatType);
    threatDetectedBuilder.setThreatName(m_threatName);
    threatDetectedBuilder.setScanType(m_scanType);
    threatDetectedBuilder.setNotificationStatus(m_notificationStatus);
    threatDetectedBuilder.setFilePath(m_filePath);
    threatDetectedBuilder.setActionCode(m_actionCode);

    if (m_filePath.empty())
    {
        // TODO: Should this be a warning?
        LOGERROR("Missing    path while serialising threat report: empty string");
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    auto reader = threatDetectedBuilder.asReader();
    if (!reader.hasThreatName())
    {
        // TODO: Should this be a warning?
        LOGERROR("Missing    did not receive a threat name in threat report");
    }

    return dataAsString;
}

