//
// Created by pair on 07/06/18.
//

#pragma once


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


