/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <optional>
#include <Common/FileSystem/IFileSystem.h>

namespace CommsComponent
{
    std::optional<Path> getCertificateStorePath();

    std::optional<Path> getCaCertificateBundleFile();

    void makeDirsAndSetPermissions(const Path& rootDir, const Path& pathRelToRootDir, const std::string& userName,
                                   const std::string& userGroup, __mode_t mode);
}  //CommsComponent
