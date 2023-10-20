// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "unixsocket/BaseClientDefaultSleepTime.h"
#include "unixsocket/metadataRescanSocket/IMetadataRescanClientSocket.h"
#include "unixsocket/restoreReportingSocket/IRestoreReportingClient.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

namespace safestore
{
    class ISafeStoreResources
    {
    public:
        using duration_t = unixsocket::duration_t;
        using SleeperSharedPtr = unixsocket::IStoppableSleeperSharedPtr;
        static constexpr auto DEFAULT_CLIENT_SLEEP_TIME = unixsocket::DEFAULT_CLIENT_SLEEP_TIME;
        virtual ~ISafeStoreResources() = default;

        virtual std::unique_ptr<unixsocket::IMetadataRescanClientSocket> CreateMetadataRescanClientSocket(
            std::string socket_path,
            const duration_t& sleepTime = DEFAULT_CLIENT_SLEEP_TIME,
            SleeperSharedPtr sleeper = {}) = 0;

        virtual std::unique_ptr<unixsocket::IScanningClientSocket> CreateScanningClientSocket(
            std::string socket_path,
            const duration_t& sleepTime = DEFAULT_CLIENT_SLEEP_TIME,
            SleeperSharedPtr sleeper = {}) = 0;

        virtual std::unique_ptr<unixsocket::IRestoreReportingClient> CreateRestoreReportingClient(
                SleeperSharedPtr sleeper = {}) = 0;
    };
} // namespace safestore