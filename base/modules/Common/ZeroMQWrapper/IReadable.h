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
    namespace ZeroMQWrapper
    {
        class IReadable : public virtual IHasFD
        {
        public:
            using data_t = std::vector<std::string>;

            virtual data_t read() = 0;
        };
    }
}

#endif //EVEREST_BASE_IREADABLE_H
