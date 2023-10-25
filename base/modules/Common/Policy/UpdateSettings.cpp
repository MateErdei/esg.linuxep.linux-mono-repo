// Copyright 2023 Sophos Limited. All rights reserved.

#include "UpdateSettings.h"

#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"

using namespace Common::Policy;

const std::vector<std::string> UpdateSettings::DefaultSophosCDNUrls{ "https://sdds3.sophosupd.com:443",
                                                                     "https://sdds3.sophosupd.net:443" };
const std::string UpdateSettings::DefaultSophosSusUrl = "https://sus.sophosupd.com";

bool UpdateSettings::isProductSubscriptionValid(const ProductSubscription& productSubscription)
{
    if (productSubscription.rigidName().empty())
    {
        LOGWARN("Invalid Settings: Empty RigidName.");
        return false;
    }
    if (productSubscription.fixedVersion().empty() && productSubscription.tag().empty())
    {
        LOGWARN("Invalid Settings: Product can not have both FixedVersion and Tag empty.");
        return false;
    }
    return true;
}

bool UpdateSettings::verifySettingsAreValid()
{
    state_ = State::FailedVerified;

    if (sophosCDNURLs_.empty())
    {
        LOGERROR("Invalid Settings: No sophos update urls provided.");
        return false;
    }
    else
    {
        for (auto& value : sophosCDNURLs_)
        {
            if (value.empty())
            {
                LOGERROR("Invalid Settings: Sophos update url provided cannot be an empty string.");
                return false;
            }
        }
    }

    if (sophosSUSUrl_.empty())
    {
        LOGERROR("Invalid Settings: No SUS URLs provided.");
        return false;
    }

    if (machineArchitecture_ == "")
    {
        LOGERROR("Invalid Settings: Unknown architecture");
        return false;
    }

    if (!isProductSubscriptionValid(getPrimarySubscription()))
    {
        LOGERROR("Invalid Settings: No primary product provided.");
        return false;
    }

    for (auto& productSubscription : getProductsSubscription())
    {
        if (!isProductSubscriptionValid(productSubscription))
        {
            return false;
        }
    }

    auto features = getFeatures();
    if (features.empty())
    {
        LOGERROR("Empty feature set");
        return false;
    }

    if (std::find(features.begin(), features.end(), "CORE") == features.end())
    {
        LOGERROR("CORE feature not in the feature set. ");
        return false;
    }

    auto esmVersion = getEsmVersion();
    if (!esmVersion.isValid())
    {
        LOGERROR("ESM feature is not valid. Name: " << esmVersion.name() << " and Token: " << esmVersion.token());
        return false;
    }

    auto fileSystem = Common::FileSystem::fileSystem();

    std::string installationRootPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    if (!fileSystem->isDirectory(installationRootPath))
    {
        LOGERROR(
            "Invalid Settings: installation root path does not exist or is not a directory: " << installationRootPath);
        return false;
    }

    std::string localWarehouseStore =
        Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseStoreDir();
    if (!fileSystem->isDirectory(localWarehouseStore))
    {
        LOGERROR(
            "Invalid Settings: Local warehouse directory does not exist or is not a directory: "
            << localWarehouseStore);
        return false;
    }

    for (auto& value : localUpdateCacheHosts_)
    {
        if (value.empty())
        {
            LOGERROR("Invalid Settings: Update cache url cannot be an empty string");
            return false;
        }
    }

    for (auto& value : installArguments_)
    {
        if (value.empty())
        {
            LOGERROR("Invalid Settings: install argument cannot be an empty string");
            return false;
        }
    }

    state_ = State::Verified;
    return true;
}

bool UpdateSettings::isVerified() const
{
    return state_ == State::Verified;
}