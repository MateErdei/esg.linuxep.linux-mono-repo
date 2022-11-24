// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <stdexcept>

class FailedToInitializeSusiException : virtual public std::runtime_error
{
public:
    explicit FailedToInitializeSusiException(const std::string& errorMsg)
        : std::runtime_error(errorMsg)
    {
    }
};
