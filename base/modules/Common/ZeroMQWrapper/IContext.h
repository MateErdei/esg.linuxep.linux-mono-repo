/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "ISocketPublisherPtr.h"
#include "ISocketRequesterPtr.h"
#include "ISocketReplierPtr.h"
#include "ISocketSubscriberPtr.h"
#include "IProxy.h"

#include <string>
#include "IContextSharedPtr.h"

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
    }
}



