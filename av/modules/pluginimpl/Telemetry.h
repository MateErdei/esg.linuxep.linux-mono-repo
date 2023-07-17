// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "Common/FileSystem/IFileSystem.h"

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace Common::SystemCallWrapper
{
    class ISystemCallWrapper;
}

namespace Plugin
{
    class Telemetry
    {
    public:
        Telemetry(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>&,
            Common::FileSystem::IFileSystem* fs);

        std::string getTelemetry();

    TEST_PUBLIC:
        static std::pair<unsigned long, unsigned long> getThreatScannerProcessinfo(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls,
            Common::FileSystem::IFileSystem* fileSystem);
        std::pair<unsigned long, unsigned long> getThreatScannerProcessinfo(
            const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);
        std::pair<unsigned long, unsigned long> getThreatScannerProcessinfo();
    private:
        std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper> syscalls_;
        Common::FileSystem::IFileSystem* filesystem_;

        static std::string getVirusDataVersion();
    };
}
