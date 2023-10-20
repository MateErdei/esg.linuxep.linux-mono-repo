// Copyright 2023 Sophos All rights reserved.

#pragma once

// Ensure this is included first - On Ubuntu 20.04 stat gives an error if not included before safestore.h
/*
In file included from /usr/include/x86_64-linux-gnu/bits/statx.h:31,
from /usr/include/x86_64-linux-gnu/sys/stat.h:446,
from /home/pair/mav/modules/safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h:16,
from /home/pair/mav/modules/safestore/Main.cpp:10:
/usr/include/linux/stat.h:59:9: error: declaration does not declare anything [-fpermissive]
59 |         __s32   __reserved;
   |         ^~~~~
 */
// clang-format off
#include <sys/stat.h>

extern "C"
{
#include <safestore.h>
}
// clang-format on

