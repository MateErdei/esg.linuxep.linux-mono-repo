// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

namespace unixsocket::updateCompleteSocket
{
    class IUpdateCompleteCallback
    {
    public:
        virtual ~IUpdateCompleteCallback() = default;
        /**
         * Called when Sophos Threat Detector has completed a SUSI update
         */
        virtual void updateComplete() = 0;
    };
}
