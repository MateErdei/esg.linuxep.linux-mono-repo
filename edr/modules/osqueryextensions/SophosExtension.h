/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include "IServiceExtension.h"

#include <OsquerySDK/OsquerySDK.h>
#include <thread>

class SophosExtension   :   public IServiceExtension
{
public:
    ~SophosExtension();
    void Start(const std::string& socket, bool verbose, std::shared_ptr<std::atomic_bool> extensionFinished) override;
    // cppcheck-suppress virtualCallInConstructor
    void Stop() override;
    int GetExitCode() override;
private:
    void Run(std::shared_ptr<std::atomic_bool> extensionFinished);

    bool m_stopped = { true };
    bool m_stopping = { false };
    std::unique_ptr<std::thread> m_runnerThread;
    OsquerySDK::Flags m_flags;
    std::unique_ptr<OsquerySDK::ExtensionInterface> m_extension;

};
