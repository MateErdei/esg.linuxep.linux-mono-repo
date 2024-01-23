// Copyright 2024 Sophos Limited. All rights reserved.

#include "SophosInstall.h"

#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>

ssize_t getSophosInstall(char* destination, size_t length)
{
    ssize_t result = readlink("/proc/self/exe", destination, length);
    if (result < 0)
    {
        return result;
    }
    if ((size_t)result == length)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    destination[result] = 0; // readlink doesn't set null terminator

    // get sbin directory
    char* sbin = dirname(destination);
    char* av = dirname(sbin);
    char* plugins = dirname(av);
    char* sophosInstall = dirname(plugins);

    if (sophosInstall != destination)
    {
        // Need to copy back if dirname hasn't returned the original buffer.
        strncpy(destination, sophosInstall, length);
    }
    return (ssize_t)strlen(destination);
}
