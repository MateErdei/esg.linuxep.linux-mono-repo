/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>

class IServiceExtension
{
public:
    virtual void Start(
        const std::string& socket,
        bool verbose,
        uintmax_t maxBatchBytes,
        unsigned int maxBatchSeconds) = 0;
    virtual void Stop() = 0;
};