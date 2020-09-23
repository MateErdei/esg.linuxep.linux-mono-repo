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

    std::vector<Path> getCaCertificateStorePaths()
    {
        std::vector<Path> caStorePaths;
        //The classic path expected by openssl is symlinked to dynamically generated
        //"/etc/pki/ca-trust/extracted" on later rhel based systems
        const std::vector<Path> caCertBundlePaths = {
                "/etc/ssl/certs/", "/etc/pki/tls/certs/", "/etc/pki/ca-trust/extracted"
        };
        auto fs = Common::FileSystem::fileSystem();
        for (const auto& caCertBundlePath : caCertBundlePaths)
        {
            if (fs->exists(caCertBundlePath))
            {
                caStorePaths.emplace_back(caCertBundlePath);
            }
        }
        return caStorePaths;
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
