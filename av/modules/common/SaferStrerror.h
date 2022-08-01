// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>

namespace common
{
    auto safer_strerror(int error) -> std::string;
}
