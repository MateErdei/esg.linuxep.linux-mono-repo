/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatDetected.h"

#include "ThreatDetected.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;

void ThreatDetected::setUserID(const std::string& userID)
{
    m_userID = userID;
}

void ThreatDetected::setDetectionTime(const std::string& detectionTime)
{
    m_detectionTime = detectionTime;
}

void ThreatDetected::setThreatType(const std::string& threatType)
{
    m_threatType = threatType;
}

void ThreatDetected::setScanType(const std::string& scanType)
{
    m_scanType = scanType;
}

void ThreatDetected::setNotificationStatus(const std::string& notificationStatus)
{
    m_notificationStatus = notificationStatus;
}

void ThreatDetected:: setThreatID(const std::string& threatID)
{
    m_threatID = threatID;
}

void ThreatDetected::setIdSource(const std::string& idSource)
{
    m_idSource = idSource;
}

void ThreatDetected::setFileName(const std::string& fileName)
{
    m_fileName = fileName;
}

void ThreatDetected::setFilePath(const std::string& filePath)
{
    m_filePath = filePath;
}

void ThreatDetected::setActionCode(const std::string& actionCode)
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
    threatDetectedBuilder.setScanType(m_scanType);
    threatDetectedBuilder.setNotificationStatus(m_notificationStatus);
    threatDetectedBuilder.setThreatID(m_threatID);
    threatDetectedBuilder.setIdSource(m_idSource);
    threatDetectedBuilder.setFileName(m_fileName);
    threatDetectedBuilder.setFilePath(m_filePath);
    threatDetectedBuilder.setActionCode(m_actionCode);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    return dataAsString;
}

