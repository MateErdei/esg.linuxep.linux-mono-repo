// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"

#include "common/AbstractThreadPluginInterface.h"

#include <atomic>
#include <condition_variable>
#include <thread>

namespace fs = sophos_filesystem;

class SafeStoreRescanWorker : public common::AbstractThreadPluginInterface
{
public:
    explicit SafeStoreRescanWorker(const fs::path& safeStoreRescanSocket);
    ~SafeStoreRescanWorker() override;

    /**
     * tryStop() uses local m_stopRequested to interrupt wait_for() in the run method.
     * This differs from normal common::AbstractThreadPluginInterface implementation as we can't use its notifyPipe to
     * request stops, as they would be consumed in the by the wait_for() check and we wouldn't be able to logically
     * check for a stop request.
     */
    void tryStop() final;
    void run() override;
    void triggerRescan();

    virtual void sendRescanRequest();

protected:
    std::mutex m_rescanLock;

private:
    static uint parseRescanIntervalConfig();

    std::condition_variable m_rescanWakeUp;
    fs::path m_safeStoreRescanSocket;
    uint m_rescanInterval; // in seconds
    std::atomic<bool> m_manualRescan = false;
    std::atomic<bool> m_stopRequested = false;
};
