// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ThreatDetected.h"

#include "Logger.h"
#include "ThreatDetected.capnp.h"

#include "datatypes/AutoFd.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;

ThreatDetected::ThreatDetected(
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
    datatypes::AutoFd autoFd) :
    m_userID(std::move(userID)),
    m_detectionTime(detectionTime),
    m_threatType(threatType),
    m_threatName(std::move(threatName)),
    m_scanType(scanType),
    m_notificationStatus(notificationStatus),
    m_filePath(std::move(filePath)),
    m_actionCode(actionCode),
    m_sha256(std::move(sha256)),
    m_threatId(std::move(threatId)),
    m_autoFd(std::move(autoFd))
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
        LOGERROR("Missing    path in threat report: empty string");
    }

    m_filePath = filePath;
}

void ThreatDetected::setActionCode(E_ACTION_CODE actionCode)
{
    m_actionCode = actionCode;
}

void ThreatDetected::setSha256(const std::string& sha256)
{
    m_sha256 = sha256;
}

void ThreatDetected::setThreatId(const std::string& threatId)
{
    m_threatId = threatId;
}

void ThreatDetected::setAutoFd(datatypes::AutoFd&& autoFd)
{
    m_autoFd = std::move(autoFd);
}

std::string ThreatDetected::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::ThreatDetected::Builder threatDetectedBuilder = message.initRoot<Sophos::ssplav::ThreatDetected>();

    threatDetectedBuilder.setUserID(m_userID);
    threatDetectedBuilder.setDetectionTime(m_detectionTime);
    threatDetectedBuilder.setThreatType(m_threatType);
    threatDetectedBuilder.setThreatName(m_threatName);
    threatDetectedBuilder.setScanType(m_scanType);
    threatDetectedBuilder.setNotificationStatus(m_notificationStatus);
    threatDetectedBuilder.setFilePath(m_filePath);
    threatDetectedBuilder.setActionCode(m_actionCode);
    threatDetectedBuilder.setSha256(m_sha256);
    threatDetectedBuilder.setThreatId(m_threatId);

    if (m_filePath.empty())
    {
        LOGERROR("Missing path while serialising threat report: empty string");
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    auto reader = threatDetectedBuilder.asReader();
    if (!reader.hasThreatName())
    {
        LOGERROR("Missing did not receive a threat name in threat report");
    }

    return dataAsString;
}

int ThreatDetected::getFd() const
{
    return m_autoFd.get();
}

datatypes::AutoFd ThreatDetected::moveAutoFd()
{
    return std::move(m_autoFd);
}