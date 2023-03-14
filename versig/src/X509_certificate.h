// Copyright 2019-2023 Sophos Limited. All Rights Reserved.

#pragma once

#include "hash.h"
#include "root_cert.h"
#include "signature.h"

#include <list>
#include <memory>
#include <string>
#include <vector>

#define VERIFY_CHAIN_OK 0
#define VERIFY_CHAIN_FAILURE 1

namespace crypto
{
    class X509_certificate_impl;

    class X509_certificate final
    {
    public:
        explicit X509_certificate(const std::string& cert_text);
        ~X509_certificate();
        X509_certificate(const X509_certificate&) = delete;
        X509_certificate& operator=(const X509_certificate&) = delete;

        void verify_signature(const std::string& body, const manifest::signature& signature);

        // Verify the certificate chain against the root certificates.
        // Throws on verification failure.
        void verify_certificate_chain(
            const std::vector<std::string>& intermediate_certs_text,
            const std::vector<root_cert>& root_certs);

    private:
        std::unique_ptr<X509_certificate_impl> data_;
    };
} // namespace crypto
