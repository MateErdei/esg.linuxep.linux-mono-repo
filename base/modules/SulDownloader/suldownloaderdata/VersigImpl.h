/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigurationData.h"
#include "IVersig.h"

#include <functional>

namespace SulDownloader::suldownloaderdata
{
    class VersigImpl : public virtual suldownloaderdata::IVersig
    {
    public:
        VerifySignature verify(
            const Common::Policy::UpdateSettings& certificate_path,
            const std::string& productDirectoryPath) const override;

    private:
        std::vector<std::string> getListOfManifestFileNames(
            const Common::Policy::UpdateSettings& configurationData,
            const std::string& productDirectoryPath) const;
    };

    using IVersigPtr = suldownloaderdata::IVersigPtr;
    using VersigCreatorFunc = std::function<IVersigPtr(void)>;

    class VersigFactory
    {
        VersigFactory();

        VersigCreatorFunc m_creator;

    public:
        static VersigFactory& instance();

        IVersigPtr createVersig();

        // for tests only
        void replaceCreator(VersigCreatorFunc creator);

        void restoreCreator();
    };
} // namespace SulDownloader::suldownloaderdata
