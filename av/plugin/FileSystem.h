/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

using Path = std::string;

class FileSystem {
public:
    std::string readFile(const Path &path) const;
    Path join(const Path& path1, const Path & path2) const;
    void makedirs(const Path &path) const;
    std::vector<Path> listFileSystemEntries(const Path &directoryPath) const;
    bool isDirectory(const Path & path) const;
    bool isRegularFile(const Path & path) const;
    Path baseName(const Path & filePath) const;
    Path dirPath(const Path & filePath) const;
};

