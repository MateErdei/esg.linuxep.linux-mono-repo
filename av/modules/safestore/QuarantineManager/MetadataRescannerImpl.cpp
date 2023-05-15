// Copyright 2023 Sophos Limited. All rights reserved.

#include "MetadataRescannerImpl.h"

#include "scan_messages/MetadataRescan.h"

namespace safestore::QuarantineManager
{
    MetadataRescanner::MetadataRescanner(SafeStoreWrapper::ISafeStoreWrapper& safeStoreWrapper) :
        safeStoreWrapper_{ safeStoreWrapper }
    {
    }

    MetadataRescanResult MetadataRescanner::Rescan(const SafeStoreWrapper::ObjectHandleHolder& objectHandle)
    {
        const auto sha256 = GetObjectCustomDataString(safeStoreWrapper_, objectHandle, "SHA256");
        const auto filePath = GetObjectPath(safeStoreWrapper_, objectHandle);
        const auto escapedPath = common::escapePathForLogging(filePath);

        LOGINFO(
            "Requesting metadata rescan of quarantined file (original path '" << escapedPath << "', object ID '"
                                                                              << objectId << "')");

        const auto threats = GetThreats(objectHandle);
        if (threats.empty())
        {
            throw SafeStoreObjectException("No threats stored");
        }

        const auto result =
            metadataRescanClientSocket.rescan({ .filePath = filePath, .sha256 = sha256, .threat = threats[0] });
        LOGDEBUG(
            "Received metadata rescan response: '" << scan_messages::MetadataRescanResponseToString(result) << "'");

        switch (result)
        {
            case scan_messages::MetadataRescanResponse::clean:
                return MetadataRescanResult::clean;
            case scan_messages::MetadataRescanResponse::needsFullScan:
            case scan_messages::MetadataRescanResponse::undetected:
            case scan_messages::MetadataRescanResponse::failed:
                return MetadataRescanResult::uncertain;
            case scan_messages::MetadataRescanResponse::threatPresent:
                return MetadataRescanResult::hasThreat;
        }

        return MetadataRescanResult::uncertain;
    }
} // namespace safestore::QuarantineManager
