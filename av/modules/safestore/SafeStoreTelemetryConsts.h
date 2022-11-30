// Copyright 2022, Sophos Limited.  All rights reserved.

namespace safestore
{
    constexpr auto telemetrySafeStoreHealth = "health";
    constexpr auto telemetrySafeStoreDormantMode = "dormant-mode";

    constexpr auto telemetrySafeStoreDatabaseSize = "database-size";

    constexpr auto telemetrySafeStoreQuarantineSuccess = "quarantine-successes";
    constexpr auto telemetrySafeStoreQuarantineFailure = "quarantine-failures";
    constexpr auto telemetrySafeStoreUnlinkFailure = "unlink-failures";

    constexpr auto telemetrySafeStoreDatabaseDeletions = "database-deletions";
    constexpr auto telemetrySafeStoreSuccessfulFileRestorations = "successful-file-restorations";
    constexpr auto telemetrySafeStoreFailedFileRestorations = "failed-file-restorations";
}