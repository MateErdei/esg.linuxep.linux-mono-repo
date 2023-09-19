// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ThreatDetected.h"

#include "Logger.h"

#include "datatypes/AutoFd.h"

#include "Common/Exceptions/IException.h"
#include "Common/UtilityImpl/Uuid.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace scan_messages;
using namespace common::CentralEnums;

ThreatDetected ThreatDetected::deserialise(Sophos::ssplav::ThreatDetected::Reader& reader)
{
    ThreatDetected threatDetected{ reader.getUserID(),
                                   reader.getDetectionTime(),
                                   reader.getThreatType(),
                                   reader.getThreatName(),
                                   static_cast<E_SCAN_TYPE>(reader.getScanType()),
                                   static_cast<QuarantineResult>(reader.getQuarantineResult()),
                                   reader.getFilePath(),
                                   reader.getSha256(),
                                   reader.getThreatSha256(),
                                   reader.getThreatId(),
                                   reader.getIsRemote(),
                                   static_cast<ReportSource>(reader.getReportSource()),
                                   reader.getPid(),
                                   reader.getExecutablePath(),
                                   reader.getCorrelationId(),
                                   datatypes::AutoFd() };

    threatDetected.validate();
    return threatDetected;
}

std::string ThreatDetected::serialise(bool validateData) const
{
    if (validateData)
    {
        validate();
    }

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::ThreatDetected::Builder threatDetectedBuilder = message.initRoot<Sophos::ssplav::ThreatDetected>();

    threatDetectedBuilder.setUserID(userID);
    threatDetectedBuilder.setDetectionTime(detectionTime);
    threatDetectedBuilder.setThreatType(threatType);
    threatDetectedBuilder.setThreatName(threatName);
    threatDetectedBuilder.setScanType(scanType);
    threatDetectedBuilder.setQuarantineResult(static_cast<int>(quarantineResult));
    threatDetectedBuilder.setFilePath(filePath);
    threatDetectedBuilder.setSha256(sha256);
    threatDetectedBuilder.setThreatSha256(threatSha256);
    threatDetectedBuilder.setThreatId(threatId);
    threatDetectedBuilder.setIsRemote(isRemote);
    threatDetectedBuilder.setReportSource(static_cast<int>(reportSource));
    threatDetectedBuilder.setPid(pid);
    threatDetectedBuilder.setExecutablePath(executablePath);
    threatDetectedBuilder.setCorrelationId(correlationId);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    return dataAsString;
}

void ThreatDetected::validate() const
{
    if (filePath.empty())
    {
        throw Common::Exceptions::IException(LOCATION, "Empty path");
    }
    if (threatName.empty())
    {
        throw Common::Exceptions::IException(LOCATION, "Empty threat name");
    }
    if (!Common::UtilityImpl::Uuid::IsValid(threatId))
    {
        throw Common::Exceptions::IException(LOCATION, "Invalid threat ID: " + threatId);
    }
    if (!correlationId.empty() && !Common::UtilityImpl::Uuid::IsValid(correlationId))
    {
        throw Common::Exceptions::IException(LOCATION, "Invalid correlation ID: " + correlationId);
    }
}

bool ThreatDetected::operator==(const ThreatDetected& other) const
{
    // clang-format off
    return userID == other.userID &&
           detectionTime == other.detectionTime &&
           threatType == other.threatType &&
           threatName == other.threatName &&
           scanType == other.scanType &&
           quarantineResult == other.quarantineResult &&
           filePath == other.filePath &&
           sha256 == other.sha256 &&
           threatSha256 == other.threatSha256 &&
           threatId == other.threatId &&
           isRemote == other.isRemote &&
           reportSource == other.reportSource &&
           pid == other.pid &&
           executablePath == other.executablePath &&
           correlationId == other.correlationId;
           // autoFd is not checked because two real autoFds should never be equal
    // clang-format on
}
