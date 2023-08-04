// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISignatureVerifierWrapper.h"

#include "sophlib/crypto/SignatureVerifier.h"

#include <filesystem>

namespace SulDownloader
{
    class SignatureVerifierWrapper : public ISignatureVerifierWrapper
    {
    public:
        explicit SignatureVerifierWrapper(const std::filesystem::path& certs_dir);

        void Verify(const sophlib::crypto::SignedBlob& data, int allowed_algorithms) const override;
        void Verify(std::string_view data, const sophlib::crypto::Signatures& signatures, int allowed_algorithms)
            const override;
        [[nodiscard]] const std::vector<sophlib::crypto::RootCertificate>& GetRootCertificates() const override;

    private:
        sophlib::crypto::SignatureVerifier verifier_;
    };
} // namespace SulDownloader