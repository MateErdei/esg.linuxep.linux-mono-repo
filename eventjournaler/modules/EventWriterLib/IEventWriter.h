/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IDataType.h>

//namespace WriterLib
//{
//    class IEventQueuePopper
//    {
//    public:
//        virtual ~IEventQueuePopper() = default;
//        IEventQueuePopper() = default;
//        virtual std::optional<Common::ZeroMQWrapper::data_t> getEvent(int timeoutInMilliseconds) = 0;
//    };
//} // namespace EventQueuePopper

namespace EventWriterLib
{
    class IEventWriter
    {
    public:
        virtual ~IEventWriter() = default;
        virtual void stop() = 0;
        virtual void start() = 0;
        virtual void restart() = 0;
        virtual bool getRunningStatus() = 0;
    };
} // namespace SubscriberLib
