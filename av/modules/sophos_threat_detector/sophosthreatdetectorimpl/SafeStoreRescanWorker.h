// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"

#include "Common/PersistentValue/PersistentValue.h"
#include "common/AbstractThreadPluginInterface.h"

#include <condition_variable>
#include <thread>

namespace fs = sophos_filesystem;

class SafeStoreRescanWorker : public common::AbstractThreadPluginInterface
{
public:
    explicit SafeStoreRescanWorker(
        const fs::path& safeStoreRescanSocket
    );
    ~SafeStoreRescanWorker() override;
    void run() override;
    void triggerRescan();

private:
    std::mutex m_rescanLock;
    std::condition_variable m_rescanWakeUp;
    fs::path m_safeStoreRescanSocket;
    Common::PersistentValue<int> m_rescanInterval;  // in seconds
    bool m_manualRescan = false;
};
