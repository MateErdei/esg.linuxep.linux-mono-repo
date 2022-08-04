// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <stdexcept>
#include <string>
#include <utility>

class PidLockFileException : public std::runtime_error
{
public:
    explicit PidLockFileException(const std::string& errorMsg)
        : std::runtime_error(errorMsg)
    {
    }
};
