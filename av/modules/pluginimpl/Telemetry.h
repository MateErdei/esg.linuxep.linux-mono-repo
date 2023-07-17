// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "Common/FileSystem/IFileSystem.h"

namespace Common::SystemCallWrapper
{
    class ISystemCallWrapper;
}

namespace Plugin
{
    class Telemetry
    {
    public:
        static std::string getTelemetry(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>&,
            Common::FileSystem::IFileSystem* fs,
            long health);
        static std::pair<unsigned long, unsigned long> getThreatScannerProcessinfo(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls,
            Common::FileSystem::IFileSystem* fileSystem);
        static std::pair<unsigned long, unsigned long> getThreatScannerProcessinfo(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);

    private:
        static std::string getVirusDataVersion();
    };
}
