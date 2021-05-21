/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

class ScanManuallyInterruptedException :  public std::runtime_error
{
public:
    explicit ScanManuallyInterruptedException(const std::string& errorMsg)
            : std::runtime_error(errorMsg)
    {
    }
};
