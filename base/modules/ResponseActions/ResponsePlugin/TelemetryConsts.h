// Copyright 2023 Sophos Limited. All rights reserved.

namespace ResponsePlugin
{
    namespace Telemetry
    {
        const char *const version = "version";
        const char* const pluginHealthStatus = "health";
        const char* const runCommandTotal = "run-command-actions";
        const char* const runCommandFails = "run-command-fails";
        const char* const runCommandT0Fails = "run-command-action-timeouts";
        const char* const runCommandEFails = "run-command-expired-actions";
    } // namespace Telemetry
} // namespace ResponsePlugin