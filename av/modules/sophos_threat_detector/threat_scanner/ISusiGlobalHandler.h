// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include <memory>
#include <string>
namespace threat_scanner
{
    class ISusiGlobalHandler
    {
    public:
        virtual ~ISusiGlobalHandler() = default;
        virtual bool isOaPuaDetectionEnabled() = 0;
        virtual bool isPuaApproved(const std::string& puaName) = 0;
        virtual bool isAllowListed(const std::string& threatChecksum) = 0;
    };
    using ISusiGlobalHandlerSharedPtr = std::shared_ptr<ISusiGlobalHandler>;
} // namespace threat_scanner
