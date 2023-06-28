// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/UpdateSettings.h"

#include <memory>
#include <string>

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
                SIGNATURE_VERIFIED,
                SIGNATURE_FAILED,
                INVALID_ARGUMENTS
            };

            using settings_t = Common::Policy::UpdateSettings;

            virtual VerifySignature verify(
                const settings_t& configurationData,
                const std::string& productDirectoryPath) const = 0;
        };

        using IVersigPtr = std::unique_ptr<IVersig>;

        IVersigPtr createVersig();
    } // namespace suldownloaderdata
} // namespace SulDownloader
