// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ThreatDetected.h"

#include "Logger.h"
#include "ThreatDetected.capnp.h"

#include "datatypes/AutoFd.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;
using namespace common::CentralEnums;

ThreatDetected::ThreatDetected(
    std::string userID,
    std::time_t detectionTime,
    ThreatType threatType,
    std::string threatName,
    E_SCAN_TYPE scanType,
    E_NOTIFCATION_STATUS notificationStatus,
    std::string filePath,
    E_ACTION_CODE actionCode,
    std::string sha256,
    std::string threatId,
    bool isRemote,
    ReportSource reportSource,
    datatypes::AutoFd autoFd) :
    userID(std::move(userID)),
    detectionTime(detectionTime),
    threatType(threatType),
    threatName(std::move(threatName)),
    scanType(scanType),
    notificationStatus(notificationStatus),
    filePath(std::move(filePath)),
    actionCode(actionCode),
    sha256(std::move(sha256)),
    threatId(std::move(threatId)),
    isRemote(isRemote),
    reportSource(reportSource),
    autoFd(std::move(autoFd))
{
}

ThreatDetected::ThreatDetected(Sophos::ssplav::ThreatDetected::Reader& reader) :
    ThreatDetected(
        reader.getUserID(),
        reader.getDetectionTime(),
        static_cast<ThreatType>(reader.getThreatType()),
        reader.getThreatName(),
        static_cast<E_SCAN_TYPE>(reader.getScanType()),
        static_cast<E_NOTIFCATION_STATUS>(reader.getNotificationStatus()),
        reader.getFilePath(),
        static_cast<E_ACTION_CODE>(reader.getActionCode()),
        reader.getSha256(),
        reader.getThreatId(),
        reader.getIsRemote(),
        static_cast<ReportSource>(reader.getReportSource()),
        datatypes::AutoFd())
{
}

std::string ThreatDetected::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::ThreatDetected::Builder threatDetectedBuilder = message.initRoot<Sophos::ssplav::ThreatDetected>();

    threatDetectedBuilder.setUserID(userID);
    threatDetectedBuilder.setDetectionTime(detectionTime);
    threatDetectedBuilder.setThreatType(static_cast<int>(threatType));
    threatDetectedBuilder.setThreatName(threatName);
    threatDetectedBuilder.setScanType(scanType);
    threatDetectedBuilder.setNotificationStatus(notificationStatus);
    threatDetectedBuilder.setFilePath(filePath);
    threatDetectedBuilder.setActionCode(actionCode);
    threatDetectedBuilder.setSha256(sha256);
    threatDetectedBuilder.setThreatId(threatId);
    threatDetectedBuilder.setIsRemote(isRemote);
    threatDetectedBuilder.setReportSource(static_cast<int>(reportSource));

    if (filePath.empty())
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

bool ThreatDetected::operator==(const ThreatDetected& other) const
{
    // clang-format off
    return userID == other.userID &&
           detectionTime == other.detectionTime &&
           threatType == other.threatType &&
           threatName == other.threatName &&
           scanType == other.scanType &&
           notificationStatus == other.notificationStatus &&
           filePath == other.filePath &&
           actionCode == other.actionCode &&
           sha256 == other.sha256 &&
           threatId == other.threatId &&
           isRemote == other.isRemote &&
           autoFd == other.autoFd &&
           reportSource == other.reportSource;
    // clang-format on
}
