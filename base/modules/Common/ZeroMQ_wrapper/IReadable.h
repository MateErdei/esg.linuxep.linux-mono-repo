//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_IREADABLE_H
#define EVEREST_BASE_IREADABLE_H

#include "IHasFD.h"

#include <vector>
#include <string>

namespace Common
{
    namespace ZeroMQ_wrapper
    {
        class IReadable : public virtual IHasFD
        {
        public:
            virtual std::vector<std::string> read() = 0;
        };
    }
}

#endif //EVEREST_BASE_IREADABLE_H
