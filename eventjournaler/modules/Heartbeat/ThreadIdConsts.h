/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

namespace Heartbeat
{
    std::string  getPluginAdapterThreadId()
    {
        return "PluginAdapter";
    }
    std::string getWriterThreadId()
    {
        return "Writer";
    }
    std::string getSubscriberThreadId()
    {
        return "Subscriber";
    }
} // namespace Heartbeat