/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

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
            verify(const std::string& certificatePath, const std::string& productDirectoryPath) const = 0;
        };

        using IVersigPtr = std::unique_ptr<IVersig>;

        IVersigPtr createVersig();
    }
}

