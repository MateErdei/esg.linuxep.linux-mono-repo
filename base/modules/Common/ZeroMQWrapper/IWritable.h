/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


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

