/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IDataType.h"
#include "IHasFD.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IWritable : public virtual IHasFD
        {
        public:
            using data_t = Common::ZeroMQWrapper::data_t;
            /**
             * Write a multi-part message from data
             */
            virtual void write(const data_t& data) = 0;
        };
    } // namespace ZeroMQWrapper
} // namespace Common
