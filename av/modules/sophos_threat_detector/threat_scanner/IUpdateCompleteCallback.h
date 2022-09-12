// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace threat_scanner
{
    class IUpdateCompleteCallback
    {
    public:
        virtual ~IUpdateCompleteCallback() = default;
        /**
         * Publish that we have completed an update
         */
        virtual void updateComplete() = 0;
    };
    using IUpdateCompleteCallbackPtr = std::shared_ptr<IUpdateCompleteCallback>;
}
