//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_COMMON__ZEROMQ_WRAPPER_ICONTEXT_H
#define EVEREST_BASE_COMMON__ZEROMQ_WRAPPER_ICONTEXT_H


#include <memory>
#include <string>

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketSubscriber;
        class ISocketPublisher;
        class ISocketRequester;
        class ISocketReplier;

        class IContext
        {
        public:
            virtual ~IContext() = default;

            virtual std::unique_ptr<ISocketSubscriber>  getSubscriber(const std::string& address) = 0;
            virtual std::unique_ptr<ISocketPublisher>   getPublisher(const std::string& address) = 0;
            virtual std::unique_ptr<ISocketRequester>   getRequester(const std::string& address) = 0;
            virtual std::unique_ptr<ISocketReplier>     getReplier(const std::string& address) = 0;
        };

        extern std::unique_ptr<IContext> createContext();
    }
}


#endif //EVEREST_BASE_COMMON__ZEROMQ_WRAPPER_ICONTEXT_H
