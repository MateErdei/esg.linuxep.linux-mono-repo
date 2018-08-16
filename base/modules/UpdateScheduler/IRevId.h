/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
namespace UpdateScheduler
{
    class IRevId
    {
    public:
        virtual ~IRevId() = default;
        virtual std::string revID() const = 0;
    };
}