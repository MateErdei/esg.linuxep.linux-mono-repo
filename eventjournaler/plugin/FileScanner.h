/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>
#include "ScanReport.h"
#include "FileSystem.h"

namespace Example
{
    class FileScanner {
        std::vector<std::string> m_exclusions;
    public:
        void setExclusions(std::vector<std::string> exclusions);
        ScanReport scan();

    private:

        void scanPathRecursively( const std::string & filePath, ScanReport& scanReport, const FileSystem & fileSystem);

        void scanFile( const std::string & filePath, ScanReport & scanReport, const FileSystem & fileSystem);

    };
}
