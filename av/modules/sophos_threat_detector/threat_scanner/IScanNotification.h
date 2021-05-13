/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace threat_scanner
{
    class IScanNotification
    {
    public:
        virtual ~IScanNotification() = default;

        virtual void reset() = 0;
        virtual time_t timeout() = 0;
    };
    using IScanNotificationSharedPtr = std::shared_ptr<IScanNotification>;
}
