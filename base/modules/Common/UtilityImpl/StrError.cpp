/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StrError.h"
#include <array>
#include <string.h>

std::string Common::UtilityImpl::StrError(int errNumber)
{
    std::array<char,256> buffer{};
    char * reference2String = ::strerror_r(errNumber, buffer.data(), buffer.size());
    return std::string(reference2String);
}
