//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_COMMON__ZEROMQ_WRAPPER_ICONTEXT_H
#define EVEREST_BASE_COMMON__ZEROMQ_WRAPPER_ICONTEXT_H

#include "ISocketRequesterPtr.h"
#include "ISocketReplierPtr.h"

#include <memory>
#include <string>

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketSubscriber;
        class ISocketPublisher;
        class ISocketReplier;

        class IContext
        {
        public:
            virtual ~IContext() = default;

            virtual std::unique_ptr<ISocketSubscriber> getSubscriber() = 0;
            virtual std::unique_ptr<ISocketPublisher> getPublisher() = 0;
            virtual ISocketRequesterPtr getRequester() = 0;
            virtual ISocketReplierPtr getReplier() = 0;
        };

        extern std::unique_ptr<IContext> createContext();
    }
}


#endif //EVEREST_BASE_COMMON__ZEROMQ_WRAPPER_ICONTEXT_H
