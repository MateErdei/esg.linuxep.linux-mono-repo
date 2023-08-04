// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "SulDownloader/sdds3/ISignatureVerifierWrapper.h"
#include "sophlib/crypto/RootCertificate.h"
#include "sophlib/crypto/Signature.h"
#include "sophlib/crypto/SignedBlob.h"

#include <gmock/gmock.h>

class MockSignatureVerifierWrapper : public SulDownloader::ISignatureVerifierWrapper
{
public:
    MOCK_METHOD(void, Verify, (const sophlib::crypto::SignedBlob& data, int allowed_algorithms), (const, override));
    MOCK_METHOD(
        void,
        Verify,
        (std::string_view data, const sophlib::crypto::Signatures& signatures, int allowed_algorithms),
        (const, override));
    MOCK_METHOD(const std::vector<sophlib::crypto::RootCertificate>&, GetRootCertificates, (), (const, override));
};