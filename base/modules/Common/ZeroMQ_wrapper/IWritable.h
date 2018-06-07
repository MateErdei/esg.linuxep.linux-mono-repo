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
    namespace ZeroMQ_wrapper
    {
        class IWritable : public virtual IHasFD
        {
        public:
            /**
             * Write a multi-part message from data
             */
            virtual void write(const std::vector<std::string>& data) = 0;
        };
    }
}
#endif //EVEREST_BASE_IWRITABLE_H
