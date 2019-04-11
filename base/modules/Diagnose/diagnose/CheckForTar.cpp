/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CheckForTar.h"

#include <stdlib.h>

bool diagnose::CheckForTar::isTarAvailable(const std::string& PATH)
{
    static_cast<void>(PATH);
    return !PATH.empty();
}

bool diagnose::CheckForTar::isTarAvailable()
{
    /**
     * We use ::getenv to match the later execp/system call, which will use the PATH,
     * even if we are called from a setuid binary.
     */
    char* PATH = ::getenv("PATH");
    return isTarAvailable(PATH);
}
