//
// Created by pair on 07/06/18.
//

#pragma once


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


