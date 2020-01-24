/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScheduledScan.h"

using namespace manager::scheduler;

manager::scheduler::ScheduledScan::ScheduledScan(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id)
{
    auto attr = savPolicy.lookup(id + "/name");
    m_name = attr.contents();
}
