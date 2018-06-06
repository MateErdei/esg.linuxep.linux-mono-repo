//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETSUBSCRIBER_H
#define EVEREST_BASE_ISOCKETSUBSCRIBER_H


namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketSubscriber
        {
        public:
            virtual ~ISocketSubscriber() = default;
        };
    }
}

#endif //EVEREST_BASE_ISOCKETSUBSCRIBER_H
