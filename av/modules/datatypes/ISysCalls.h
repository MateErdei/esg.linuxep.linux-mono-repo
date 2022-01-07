/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <memory>
#include <utility>

namespace datatypes
{
    class ISysCalls
    {
    public:
        inline ISysCalls() = default;

        inline virtual ~ISysCalls() = default;

        virtual std::pair<const int, const long> getSystemUpTime() = 0;
    };
}