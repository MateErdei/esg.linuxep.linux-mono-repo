/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

class ScanInterruptedException : public std::runtime_error
{
public:
    explicit ScanInterruptedException(const std::string& errorMsg)
            : std::runtime_error(errorMsg)
    {
    }
};
