//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_BASE_ISOCKETREQUESTER_H
#define EVEREST_BASE_ISOCKETREQUESTER_H

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class ISocketRequester
        {
        public:
            virtual ~ISocketRequester() = default;
        };
    }
}

#endif //EVEREST_BASE_ISOCKETREQUESTER_H
