/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ConfigurationData.h"

#include <string>
#include <memory>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class IVersig
        {
        public:
            virtual ~IVersig() = default;

            enum class VerifySignature
            {
                SIGNATURE_VERIFIED, SIGNATURE_FAILED, INVALID_ARGUMENTS
            };

            virtual VerifySignature
            verify(const ConfigurationData& configurationData, const std::string& productDirectoryPath) const = 0;
        };

        using IVersigPtr = std::unique_ptr<IVersig>;

        IVersigPtr createVersig();
    }
}

