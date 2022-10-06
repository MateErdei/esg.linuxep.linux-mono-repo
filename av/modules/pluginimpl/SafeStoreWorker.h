// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include "DetectionQueue.h"
#include "IDetectionReportProcessor.h"

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"

#include "common/AbstractThreadPluginInterface.h"

#include <atomic>
#include <optional>
#include <thread>

namespace fs = sophos_filesystem;

namespace Plugin
{
    class SafeStoreWorker : public common::AbstractThreadPluginInterface
    {
    public:
        explicit SafeStoreWorker(
            const IDetectionReportProcessor& pluginAdapter,
            std::shared_ptr<DetectionQueue> detectionQueue,
            const fs::path& safeStoreSocket
        );
        void run() override;

    private:
        const IDetectionReportProcessor& m_pluginAdapter;
        std::shared_ptr<DetectionQueue> m_detectionQueue;
        fs::path m_safeStoreSocket;
    };
}
