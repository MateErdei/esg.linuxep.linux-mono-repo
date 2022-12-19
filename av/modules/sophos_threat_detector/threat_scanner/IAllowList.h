// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include <memory>
#include <string>

namespace threat_scanner
{
    class IAllowList
    {
    public:
        virtual ~IAllowList() = default;
        virtual bool isAllowListed(const std::string& threatChecksum) = 0;
    };

    using IAllowListSharedPtr = std::shared_ptr<IAllowList>;
}
