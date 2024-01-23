// Copyright 2024 Sophos Limited. All rights reserved.
#ifndef ESG_LINUXEP_LINUX_MONO_REPO_SOPHOSINSTALL_H
#define ESG_LINUXEP_LINUX_MONO_REPO_SOPHOSINSTALL_H


#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h>

/**
 *
 * Get SOPHOS_INSTALL from /proc/self/exe
 *
 * Not thread-safe
 *
 * @param destination
 * @param length
 * @return Size of Sophos Install, or -1 and errno for errors
 * ENAMETOOLONG - destination buffer not big enough
 */
ssize_t getSophosInstall(char* destination, size_t length);


#ifdef __cplusplus
} // extern "C"
#endif


#endif //ESG_LINUXEP_LINUX_MONO_REPO_SOPHOSINSTALL_H
