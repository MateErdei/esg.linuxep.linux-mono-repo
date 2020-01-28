/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <iostream>
#if __cplusplus >= 201703L && __has_include(<filesystem>)
#include <filesystem>
namespace sophos_filesystem = std::filesystem;
#else
#include <experimental/filesystem>
namespace sophos_filesystem = std::experimental::filesystem;
#endif
