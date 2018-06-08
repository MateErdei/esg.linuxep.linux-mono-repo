//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_IHASFD_H
#define EVEREST_BASE_IHASFD_H

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IHasFD
        {
        public:
            virtual ~IHasFD() = default;
            virtual int fd() = 0;
            virtual void setNonBlocking() = 0;
        };
    }
}

#endif //EVEREST_BASE_IHASFD_H
