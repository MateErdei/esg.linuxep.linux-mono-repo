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
        class IReadable : public virtual IHasFD
        {
        public:
            using data_t = Common::ZeroMQWrapper::data_t;

            virtual data_t read() = 0;
        };
    } // namespace ZeroMQWrapper
} // namespace Common
