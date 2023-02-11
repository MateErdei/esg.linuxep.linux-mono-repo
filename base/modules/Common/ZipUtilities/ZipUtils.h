// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/FileSystem/IFileSystem.h>

namespace Common::ZipUtilities
{

    class ZipUtils
    {
    public:
        static int zip(const std::string& srcPath, const std::string& destPath, bool passwordProtected=false, const std::string& password="");
        static int unzip(const std::string& srcPath, const std::string& destPath, bool passwordProtected=false, const std::string& password="");
    };

} // namespace Common::ZipUtilities


