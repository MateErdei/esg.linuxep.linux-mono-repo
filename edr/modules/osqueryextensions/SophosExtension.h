/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <OsquerySDK/OsquerySDK.h>
#include <thread>

class SophosExtension
{
public:
    ~SophosExtension();
    void Start(const std::string& socket, bool verbose);
    void Stop();

private:
    void Run();

    bool m_stopped = { true };
    std::unique_ptr<std::thread> m_runnerThread;
    OsquerySDK::Flags m_flags;
    std::unique_ptr<OsquerySDK::ExtensionInterface> m_extension;

};
