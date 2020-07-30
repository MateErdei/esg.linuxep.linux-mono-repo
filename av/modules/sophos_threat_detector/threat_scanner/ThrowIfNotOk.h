/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Susi.h>

#include <stdexcept>
#include <string>

inline void throwIfNotOk(SusiResult res, const std::string& message)
{
    if (res != SUSI_S_OK)
    {
        throw std::runtime_error(message);
    }
}
