// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "IAllowList.h"

namespace threat_scanner
{
    class ISusiGlobalHandler : public IAllowList
    {
    public:
        virtual bool isOaPuaDetectionEnabled() = 0;
        virtual bool isOdPuaDetectionEnabled() = 0;
    };
    using ISusiGlobalHandlerSharedPtr = std::shared_ptr<ISusiGlobalHandler>;
}
