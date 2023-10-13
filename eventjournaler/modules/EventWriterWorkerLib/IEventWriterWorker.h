/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IDataType.h>

namespace EventWriterLib
{
    class IEventWriterWorker
    {
    public:
        virtual ~IEventWriterWorker() = default;
        virtual void stop() = 0;
        virtual void start() = 0;
        virtual void restart() = 0;
        virtual bool getRunningStatus() = 0;
    };
} // namespace EventWriterLib
