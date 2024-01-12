// Copyright 2024 Sophos All rights reserved.

#pragma once

#include "IsolateResult.h"

#include <vector>
#include <memory>

namespace Plugin
{
    class IsolationExclusion;

    class INftWrapper
    {
    public:
        virtual ~INftWrapper() = default;
        // Isolate the endpoint and allow the specified IP exclusions
        virtual IsolateResult applyIsolateRules(const std::vector<Plugin::IsolationExclusion>& allowList) = 0;

        // Flush all Sophos isolation rules
        virtual IsolateResult clearIsolateRules() = 0;

    };

    using INftWrapperPtr = std::shared_ptr<INftWrapper>;
}