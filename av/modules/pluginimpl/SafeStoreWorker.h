// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include "IDetectionReportProcessor.h"
#include "QueueSafeStoreTask.h"

#include "unixsocket/safeStoreSocket/SafeStoreClientSocket.h"

#include "common/AbstractThreadPluginInterface.h"
#include "datatypes/sophos_filesystem.h"

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
            std::shared_ptr<QueueSafeStoreTask> safeStoreQueue,
            const fs::path& safeStoreSocket
        );
        void run() override;

    private:
        const IDetectionReportProcessor& m_pluginAdapter;
        std::shared_ptr<QueueSafeStoreTask> m_safeStoreQueue;
        fs::path m_safeStoreSocket;
    };
}
