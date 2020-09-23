/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/UtilityImpl/StringUtils.h>
#include "CommsComponentUtils.h"


namespace CommsComponent
{
    std::optional<Path> getCertificateStorePath()
    {
        const std::vector<Path> caStorePaths = {"/etc/ssl/certs", "/etc/pki/tls/certs"};
        for (const auto& caStorePath : caStorePaths)
        {
            if (Common::FileSystem::fileSystem()->isDirectory(caStorePath))
            {
                return caStorePath;
            }
        }
        return std::nullopt;
    }

    std::optional<Path> getCaCertificateBundleFile()
    {
        const std::vector<Path> caCertBundlePaths = {
                "/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt"
        };
        for (const auto& caCertBundlePath : caCertBundlePaths)
        {
            if (Common::FileSystem::fileSystem()->isFile(caCertBundlePath))
            {
                return caCertBundlePath;
            }
        }
        return std::nullopt;
    }

    void makeDirsAndSetPermissions(const Path& rootDir, const Path& pathRelToRootDir, const std::string& userName,
                                   const std::string& userGroup, __mode_t mode)
    {
        auto directoriesToAdd = Common::UtilityImpl::StringUtils::splitString(pathRelToRootDir, "/");
        if (directoriesToAdd.empty())
        {
            directoriesToAdd.emplace_back(pathRelToRootDir);
        }


        auto fs = Common::FileSystem::fileSystem();
        auto ifperms = Common::FileSystem::FilePermissionsImpl();
        auto newRoot = rootDir;
        for (const auto& dirName : directoriesToAdd)
        {

            newRoot = Common::FileSystem::join(newRoot, dirName);

            if (fs->isDirectory(newRoot))
            {
                continue;
            }

            fs->makedirs(newRoot);
            ifperms.chmod(newRoot, mode);
            ifperms.chown(newRoot, userName, userGroup);
        }
    }
}       //CommsComponent
