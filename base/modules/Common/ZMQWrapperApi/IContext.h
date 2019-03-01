/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IContextSharedPtr.h"
#include "Common/ZeroMQWrapper/IProxy.h"
#include "Common/ZeroMQWrapper/ISocketPublisherPtr.h"
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"
#include "Common/ZeroMQWrapper/ISocketSubscriberPtr.h"

#include <string>

namespace Common
{
    namespace ZMQWrapperApi
    {
        class IContext
        {
        public:
            virtual ~IContext() = default;

            virtual ZeroMQWrapper::ISocketSubscriberPtr getSubscriber() = 0;
            virtual ZeroMQWrapper::ISocketPublisherPtr getPublisher() = 0;
            virtual ZeroMQWrapper::ISocketRequesterPtr getRequester() = 0;
            virtual ZeroMQWrapper::ISocketReplierPtr getReplier() = 0;
            virtual ZeroMQWrapper::IProxyPtr getProxy(const std::string& frontend, const std::string& backend) = 0;
        };

        extern ZMQWrapperApi::IContextSharedPtr createContext();
    } // namespace ZeroMQWrapper
} // namespace Common
