//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETPUBLISHER_H
#define EVEREST_BASE_ISOCKETPUBLISHER_H

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketPublisher
        {
        public:
            virtual ~ISocketPublisher() = default;
        };
    }
}

#endif //EVEREST_BASE_ISOCKETPUBLISHER_H
