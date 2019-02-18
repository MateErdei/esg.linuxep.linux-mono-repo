/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IContextSharedPtr.h"
#include "IProxy.h"
#include "ISocketPublisherPtr.h"
#include "ISocketReplierPtr.h"
#include "ISocketRequesterPtr.h"
#include "ISocketSubscriberPtr.h"

#include <string>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IContext
        {
        public:
            virtual ~IContext() = default;

            virtual ISocketSubscriberPtr getSubscriber() = 0;
            virtual ISocketPublisherPtr getPublisher() = 0;
            virtual ISocketRequesterPtr getRequester() = 0;
            virtual ISocketReplierPtr getReplier() = 0;
            virtual IProxyPtr getProxy(const std::string& frontend, const std::string& backend) = 0;
        };

        extern IContextSharedPtr createContext();
    } // namespace ZeroMQWrapper
} // namespace Common
