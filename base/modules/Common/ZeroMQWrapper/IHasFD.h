//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_IHASFD_H
#define EVEREST_BASE_IHASFD_H

#include <memory>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IHasFD
        {
        public:
            virtual ~IHasFD() = default;
            virtual int fd() = 0;
        };

        using IHasFDPtr = std::unique_ptr<IHasFD>;
    }
}

#endif //EVEREST_BASE_IHASFD_H
