// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/FileSystem/IFileSystem.h>

namespace Common::ZipUtilities
{

    class ZipUtils
    {
    public:
        /*
        * Archive the diagnose output into a zip ready for upload to Central location
         */
        void produceZip(const std::string& srcPath, const std::string& destPath) const;
    };

} // namespace Common::ZipUtilities


