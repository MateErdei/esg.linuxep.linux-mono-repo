// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

extern "C"
{
#include <sys/fanotify.h>
}

namespace
{
    inline constexpr unsigned int EXPECTED_FANOTIFY_FLAGS =
        FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_UNLIMITED_MARKS | FAN_NONBLOCK;
}
