// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SaferStrerror.h"

#include <cstring>

auto common::safer_strerror(int error) -> std::string
{
    errno = 0;
    char buf[256];
#ifdef _GNU_SOURCE
    const char* res = strerror_r(error, buf, sizeof(buf));
    return res;
#else
    int res = strerror_r(error, buf, sizeof(buf));
    std::ignore = res;
    return buf;
#endif
}
