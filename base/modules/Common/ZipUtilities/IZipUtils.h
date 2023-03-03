// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <memory>
#include <string>
namespace Common::ZipUtilities
{

    class IZipUtils
    {
    public:
        virtual ~IZipUtils() = default;
        [[nodiscard]] virtual int zip(const std::string& srcPath, const std::string& destPath, bool ignoreFileError)
            const = 0;
        [[nodiscard]] virtual int zip(
            const std::string& srcPath,
            const std::string& destPath,
            bool ignoreFileError,
            bool passwordProtected) const = 0;
        [[nodiscard]] virtual int zip(
            const std::string& srcPath,
            const std::string& destPath,
            bool ignoreFileError,
            bool passwordProtected,
            const std::string& password) const = 0;
        [[nodiscard]] virtual int unzip(const std::string& srcPath, const std::string& destPath) const = 0;
        [[nodiscard]] virtual int unzip(
            const std::string& srcPath,
            const std::string& destPath,
            bool passwordProtected,
            const std::string& password) const = 0;
    };
    IZipUtils& zipUtils();
    /** Use only for test */
    void replaceZipUtils(std::unique_ptr<IZipUtils>);
    void restoreZipUtils();
} // namespace Common::ZipUtilities