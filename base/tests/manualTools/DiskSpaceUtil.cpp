// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/FileSystemImpl/FileSystemImpl.h"

#include <iostream>

int main(int argc, char* argv[])
{
    auto filesystem = std::make_unique<Common::FileSystem::FileSystemImpl>();
    if (argc == 2)
    {
        std::string mountPoint(argv[1]);
        std::filesystem::space_info diskSpaceInfo = filesystem->getDiskSpaceInfo(mountPoint);
        std::cout << "Disk space info for " << mountPoint << std::endl;
        std::cout << "   Total size: " << diskSpaceInfo.capacity << std::endl;
        std::cout << "   Free space: " << diskSpaceInfo.free << std::endl;
        std::cout << "   Available space: " << diskSpaceInfo.available << std::endl;
        return EXIT_SUCCESS;
    }
    else
    {
        std::cerr << " Invalid argument. Usage: " << argv[0] << " <path>" << std::endl;
        return EINVAL;
    }
}
