// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "IZipUtils.h"

namespace Common::ZipUtilities
{

    class ZipUtils : public virtual IZipUtils
    {
    public:
        [[nodiscard]] int zip(const std::string& srcPath, const std::string& destPath, bool ignoreFileError)
            const override;
        [[nodiscard]] int zip(
            const std::string& srcPath,
            const std::string& destPath,
            bool ignoreFileError,
            bool passwordProtected) const override;
        [[nodiscard]] int zip(
            const std::string& srcPath,
            const std::string& destPath,
            bool ignoreFileError,
            bool passwordProtected,
            const std::string& password) const override;
        [[nodiscard]] int unzip(const std::string& srcPath, const std::string& destPath) const override;
        [[nodiscard]] int unzip(
            const std::string& srcPath,
            const std::string& destPath,
            bool passwordProtected,
            const std::string& password) const override;
    };

} // namespace Common::ZipUtilities
