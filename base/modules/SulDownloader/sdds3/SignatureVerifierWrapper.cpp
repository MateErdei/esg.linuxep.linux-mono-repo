// Copyright 2023 Sophos Limited. All rights reserved.

#include "SignatureVerifierWrapper.h"

#include <filesystem>

namespace SulDownloader
{
    SignatureVerifierWrapper::SignatureVerifierWrapper(const std::filesystem::path& certs_dir) : verifier_{ certs_dir }
    {
    }

    void SignatureVerifierWrapper::Verify(const sophlib::crypto::SignedBlob& data, int allowed_algorithms) const
    {
        verifier_.Verify(data, allowed_algorithms);
    }

    void SignatureVerifierWrapper::Verify(
        std::string_view data,
        const sophlib::crypto::Signatures& signatures,
        int allowed_algorithms) const
    {
        verifier_.Verify(data, signatures, allowed_algorithms);
    }

    const std::vector<sophlib::crypto::RootCertificate>& SignatureVerifierWrapper::GetRootCertificates() const
    {
        return verifier_.GetRootCertificates();
    }
} // namespace SulDownloader