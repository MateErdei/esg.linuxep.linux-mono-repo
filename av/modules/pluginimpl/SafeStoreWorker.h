// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "DetectionQueue.h"
#include "IDetectionHandler.h"

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/sophos_filesystem.h"
#include "mount_monitor/mountinfo/IMountFactory.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"

#include <atomic>
#include <optional>
#include <thread>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace fs = sophos_filesystem;

namespace Plugin
{
    class SafeStoreWorker : public common::AbstractThreadPluginInterface
    {
    public:
        SafeStoreWorker(
            IDetectionHandler& detectionHandler,
            std::shared_ptr<DetectionQueue> detectionQueue,
            fs::path safeStoreSocket
        );
        void run() override;
    TEST_PUBLIC:
        SafeStoreWorker(
            IDetectionHandler& detectionHandler,
            std::shared_ptr<DetectionQueue> detectionQueue,
            fs::path safeStoreSocket,
            mount_monitor::mountinfo::IMountFactorySharedPtr mountFactory
        );

    private:
        IDetectionHandler& m_detectionHandler;
        std::shared_ptr<DetectionQueue> m_detectionQueue;
        fs::path m_safeStoreSocket;
        mount_monitor::mountinfo::IMountFactorySharedPtr mountFactory_;
    };
}
