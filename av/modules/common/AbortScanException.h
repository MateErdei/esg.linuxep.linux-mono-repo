/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

class AbortScanException : virtual public std::runtime_error
{
public:
    explicit AbortScanException(const std::string& errorMsg)
        : std::runtime_error(errorMsg)
    {
    }
};
