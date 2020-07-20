/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include "CertficateStoreUtils.h"


namespace CommsComponent
{
    std::optional<std::string> getCertificateStorePath()
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

    std::optional<std::string> getCaCertificateBundleFile()
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
}       //CommsComponent
