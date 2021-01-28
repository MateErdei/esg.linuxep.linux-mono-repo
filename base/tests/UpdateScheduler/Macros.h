/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#define MULTI_LINE_MACRO_BEGIN                                                                                                     \
    do                                                                                                                             \
    {
#define MULTI_LINE_MACRO_END                                                                                                       \
    __pragma(warning(suppress : 4127))                                                                                             \
    }                                                                                                                              \
    while (0)