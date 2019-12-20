//
// Created by pair on 20/12/2019.
//

#ifndef SSPL_PLUGIN_MAV_SOPHOS_FILESYSTEM_H
#define SSPL_PLUGIN_MAV_SOPHOS_FILESYSTEM_H

#include <iostream>
#if __cplusplus >= 201703L && __has_include(<filesystem>)
#include <filesystem>
namespace sophos_filesystem = std::filesystem;
#else
#include <experimental/filesystem>
namespace sophos_filesystem = std::experimental::filesystem;
#endif

#endif //SSPL_PLUGIN_MAV_SOPHOS_FILESYSTEM_H
