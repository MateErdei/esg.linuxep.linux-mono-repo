// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "IAllowList.h"

namespace threat_scanner
{
    class ISusiGlobalHandler : public IAllowList
    {
    };
    using ISusiGlobalHandlerSharedPtr = std::shared_ptr<ISusiGlobalHandler>;
}
