// Copyright 2021 Sophos Limited. All rights reserved.
#pragma once

namespace SubscriberLib
{
    class ISubscriber
    {
    public:
        virtual ~ISubscriber() = default;
        virtual void stop() = 0;
        virtual void start() = 0;
        virtual void restart() = 0;
        virtual bool getRunningStatus() = 0;
    };
} // namespace SubscriberLib
