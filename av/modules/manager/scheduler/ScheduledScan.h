/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/XmlUtilities/AttributesMap.h>

#include <string>

namespace manager::scheduler
{
    class ScheduledScan
    {
    public:
        ScheduledScan(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id);
        std::string name()
        {
            return m_name;
        }

    private:
        std::string m_name;
    };
}



