/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
namespace Common::UtilityImpl
{
    /// Implements a thread safe retrieval of strerror
    std::string StrError(int errNumber);
} // namespace Common::UtilityImpl
