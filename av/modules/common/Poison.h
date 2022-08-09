// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#ifndef SOPHOS_NO_POISON
#ifdef __GNUC__
// Include algorithm first, as it has rand and srand in it
// Include cstdio first, as it has sprintf and vsprintf in it
// Include cstdlib first, as it has mktemp in it
// Include cstring first, as it has strcpy in it
// Include ctime first, as it has asctime and localtime in it
// Include cwchar first, as it has wcscpy in it
// Include netdb first, as it has rexec and rexec_af in it
// Include string first, as it has setlocale in it
// Include unistd first, as it has getwd, getpass and getlogin in it
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <string>
#include <netdb.h>
#include <unistd.h>
// String handling functions
#pragma GCC poison strcpy wcscpy stpcpy wcpcpy
#pragma GCC poison scanf sscanf vscanf fwscanf swscanf wscanf
#pragma GCC poison gets puts
#pragma GCC poison strcat wcscat
#pragma GCC poison wcrtomb wctob
//#pragma GCC poison sprintf - boost/include/boost/assert/source_location.hpp
#pragma GCC poison vsprintf vfprintf
#pragma GCC poison asprintf vasprintf
#pragma GCC poison strncpy wcsncpy
#pragma GCC poison strtok wcstok
#pragma GCC poison strerror
#pragma GCC poison mbrlen mbsrtowcs mbrtowc wcrtomb wcsrtombs
// Time related
#pragma GCC poison asctime localtime
// Signal related
#pragma GCC poison longjmp siglongjmp
#pragma GCC poison setjmp sigsetjmp
// Memory allocation
#pragma GCC poison mallopt
// File API's
//#pragma GCC poison remove - /usr/include/x86_64-linux-gnu/sys/sysmacros.h
#pragma GCC poison mktemp tmpnam tempnam
#pragma GCC poison getwd
// Misc
#pragma GCC poison getlogin getpass cuserid
#pragma GCC poison rexec rexec_af
#pragma GCC poison rand srand
#pragma GCC poison setlocale
//#pragma GCC poison localeconv - thirdparty/nlohmann-json/json.hpp
#endif /* __GNUC__ */
#endif /* SOPHOS_NO_POISON */
