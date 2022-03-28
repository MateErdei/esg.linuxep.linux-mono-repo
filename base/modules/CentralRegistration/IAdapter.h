/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace CentralRegistrationImpl
{
    class IAdapter
    {
    public:
        virtual ~IAdapter() = default;

        virtual std::string getStatusXml() const = 0;
    };
}// namespace OSUtilities
