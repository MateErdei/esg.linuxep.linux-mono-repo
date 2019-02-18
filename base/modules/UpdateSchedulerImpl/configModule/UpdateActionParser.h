/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        bool isUpdateNowAction(const std::string& actionXml);

    }
} // namespace UpdateSchedulerImpl
