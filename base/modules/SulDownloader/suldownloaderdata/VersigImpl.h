/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigurationData.h"
#include "IVersig.h"

#include <functional>
namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class VersigImpl : public virtual suldownloaderdata::IVersig
        {
        public:
            VerifySignature verify(
                const SulDownloader::suldownloaderdata::ConfigurationData& certificate_path,
                const std::string& productDirectoryPath) const override;
        private:
            const std::vector<std::string> getListOfManifestFileNames(
                    const ConfigurationData& configurationData,
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
    } // namespace suldownloaderdata
} // namespace SulDownloader
