/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace UpdateSchedulerImpl::configModule
{
    bool isUpdateNowAction(const std::string& actionXml);
} // namespace UpdateSchedulerImpl::configModule
