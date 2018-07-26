//
// Created by pair on 06/06/18.
//

#pragma once


#include "ISocketPublisherPtr.h"
#include "ISocketRequesterPtr.h"
#include "ISocketReplierPtr.h"
#include "ISocketSubscriberPtr.h"

#include <string>
#include "IContextPtr.h"

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
        };

        extern IContextPtr createContext();
    }
}



