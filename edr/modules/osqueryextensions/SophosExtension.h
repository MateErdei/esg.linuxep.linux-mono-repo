// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once
#include "IServiceExtension.h"

#include "Common/Threads/LockableData.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

#include <boost/thread.hpp>

class SophosExtension : public IServiceExtension
{
public:
    ~SophosExtension() override;
    void Start(const std::string& socket, bool verbose, std::shared_ptr<std::atomic_bool> extensionFinished) override;
    // cppcheck-suppress virtualCallInConstructor
    void Stop(long timeoutSeconds) final;
    int GetExitCode() override;
private:
    void Run(const std::shared_ptr<std::atomic_bool>& extensionFinished);

    bool m_stopped = { true };
    Common::Threads::LockableData<bool> stopping_{false};
    std::unique_ptr<boost::thread> m_runnerThread;
    OsquerySDK::Flags m_flags;
    std::unique_ptr<OsquerySDK::ExtensionInterface> m_extension;

    [[nodiscard]] bool stopping()
    {
        auto locked = stopping_.lock();
        return *locked;
    }

    void stopping(bool value)
    {
        auto locked = stopping_.lock();
        *locked = value;
    }
};
