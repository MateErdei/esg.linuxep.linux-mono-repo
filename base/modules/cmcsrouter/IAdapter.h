/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <map>

namespace MCS
{
    class IAdapter
    {
    public:
        virtual ~IAdapter() = default;

        virtual std::string getStatusXml(std::map<std::string, std::string>& configOptions) const = 0;
    };
}
