// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "DetectionQueue.h"
#include "IDetectionHandler.h"

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"

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
            IDetectionHandler& detectionHandler,
            std::shared_ptr<DetectionQueue> detectionQueue,
            const fs::path& safeStoreSocket
        );
        void run() override;

    private:
        IDetectionHandler& m_detectionHandler;
        std::shared_ptr<DetectionQueue> m_detectionQueue;
        fs::path m_safeStoreSocket;
    };
}
