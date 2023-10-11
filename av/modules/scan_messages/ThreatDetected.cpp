/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatDetected.h"

#include "ThreatDetected.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;


ThreatDetected::ThreatDetected()
        : m_threatType(E_VIRUS_THREAT_TYPE)
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

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    return dataAsString;
}

