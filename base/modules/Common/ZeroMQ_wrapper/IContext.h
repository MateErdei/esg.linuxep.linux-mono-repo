//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_COMMON_ZEROMQ_WRAPPER_ICONTEXT_H
#define EVEREST_BASE_COMMON_ZEROMQ_WRAPPER_ICONTEXT_H

#include "ISocketPublisherPtr.h"
#include "ISocketRequesterPtr.h"
#include "ISocketReplierPtr.h"
#include "ISocketSubscriberPtr.h"

#include <string>
#include "IContextPtr.h"

namespace Common
{
    namespace ZeroMQ_wrapper
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


#endif //EVEREST_BASE_COMMON_ZEROMQ_WRAPPER_ICONTEXT_H
