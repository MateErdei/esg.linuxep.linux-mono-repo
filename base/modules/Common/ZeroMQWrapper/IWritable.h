//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_BASE_IWRITABLE_H
#define EVEREST_BASE_IWRITABLE_H

#include "IHasFD.h"

#include <vector>
#include <string>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IWritable : public virtual IHasFD
        {
        public:
            using data_t = std::vector<std::string>;
            /**
             * Write a multi-part message from data
             */
            virtual void write(const data_t& data) = 0;
        };
    }
}
#endif //EVEREST_BASE_IWRITABLE_H
