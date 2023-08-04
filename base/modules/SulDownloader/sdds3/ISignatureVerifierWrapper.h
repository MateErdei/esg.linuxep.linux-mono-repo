// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophlib/crypto/RootCertificate.h"
#include "sophlib/crypto/Signature.h"
#include "sophlib/crypto/SignedBlob.h"

namespace SulDownloader
{
    class ISignatureVerifierWrapper
    {
    public:
        virtual ~ISignatureVerifierWrapper() = default;

        virtual void Verify(const sophlib::crypto::SignedBlob& data, int allowed_algorithms) const = 0;
        virtual void Verify(
            std::string_view data,
            const sophlib::crypto::Signatures& signatures,
            int allowed_algorithms) const = 0;
        [[nodiscard]] virtual const std::vector<sophlib::crypto::RootCertificate>& GetRootCertificates() const = 0;
    };
} // namespace SulDownloader