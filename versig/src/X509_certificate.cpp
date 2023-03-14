// Copyright 2019-2023 Sophos Limited. All Rights Reserved.

#include "X509_certificate.h"

#include "crypto_utils.h"
#include "libcryptosupport.h"
#include "print.h"
#include "signed_file.h"
#include "verify_exceptions.h"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
    using DWORD = uint32_t;
}


namespace crypto
{
    class X509_certificate_impl
    {
    public:
        explicit X509_certificate_impl(const std::string& cert_text)
            : cert_(nullptr, ::X509_free)
        {
            cert_.reset(VerificationToolCrypto::X509_decode(cert_text));
            if (!cert_)
            {
                throw verify_exceptions::ve_crypt("Decoding certificate");
            }
        }
        X509_certificate_impl(const X509_certificate_impl&) = delete;
        X509_certificate_impl(X509_certificate_impl&&) = default;
        X509_certificate_impl& operator=(const X509_certificate_impl&) = delete;
        X509_certificate_impl& operator=(X509_certificate_impl&&) = default;

        X509_ptr cert_;

        [[nodiscard]] X509* get() const
        {
            return cert_.get();
        }
    };

    X509_certificate::X509_certificate(const std::string& cert_text) :
        data_{ std::make_unique<X509_certificate_impl>(cert_text) }
    {
    }

    X509_certificate::~X509_certificate() = default;

    static X509_certificate_impl X509_decode(const std::string& text)
    {
        return X509_certificate_impl{text};
    }

    static std::string decode_raw_signature(const std::string& base64_signature)
    {
        return VerificationToolCrypto::base64_decode(base64_signature);
    }

    void X509_certificate::verify_signature(const std::string& body, const manifest::signature& signature)
    {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        auto* algorithm =  crypto::construct_digest_algorithm(signature.algo_);
        EVP_VerifyInit(ctx, algorithm);
        EVP_VerifyUpdate(ctx, body.c_str(), signature.body_length_);


        // extract public key from certificate
        EVP_PKEY* pubkey = X509_get_pubkey(data_->get());
        if (!pubkey)
        {
            throw verify_exceptions::ve_crypt("Getting public key from certificate");
        }
        KeyFreer FreeKey(pubkey);

        auto raw_signature = decode_raw_signature(signature.signature_);
        int result = EVP_VerifyFinal(ctx, (unsigned char*)(raw_signature.c_str()), raw_signature.length(), pubkey);

        EVP_MD_CTX_free(ctx);

        switch (result)
        {
            case 1:
                return;
            case 0:
                throw verify_exceptions::ve_badsig();
            default:
                throw verify_exceptions::ve_crypt("Error verifying signature");
        }
        // -1 for error, 0 for bad sig, 1 for verified sig
    }

    static int verify_certificate_chain_single_root(
        const X509_certificate_impl& sig_cert,
        const X509StackWrapper& untrusted_certs_stack,
        X509_ptr& root_cert
    )
    {
        X509StoreWrapper store;

        if (!root_cert.get())
        {
            return 0;
        }
        int result = X509_STORE_add_cert(store.GetPtr(), root_cert.get());
        if (!result)
        {
            throw verify_exceptions::ve_crypt("Error loading trusted certificates file");
        }

        // Verify against this certificate.
        X509StoreCtxWrapper verify_ctx;

        if (X509_STORE_CTX_init(verify_ctx.GetPtr(), store.GetPtr(), sig_cert.get(), untrusted_certs_stack.GetPtr()) != 1)
        {
            throw verify_exceptions::ve_crypt("Error initialising verification context");
        }

        return X509_verify_cert(verify_ctx.GetPtr());
    }

    static bool verify_certificate_chain_impl(
            const X509_certificate_impl& sig_cert,
            const X509StackWrapper& untrusted_certs_stack,
            const root_cert& root_cert
            )
    {
        BIOWrapper in(root_cert.pem_crt);
        if (!in.valid())
        {
            throw verify_exceptions::ve_crypt("Error opening trusted certificates file");
        }
        int count = 0;
        for (;;)
        {
            X509_ptr this_cert{PEM_read_bio_X509(in.get(), NULLPTR, NULLPTR, NULLPTR),  ::X509_free};
            if (!this_cert)
            {
                break;
            }
            count++;
            int status = verify_certificate_chain_single_root(sig_cert, untrusted_certs_stack, this_cert);
            switch (status)
            {
                case 1:
                    return true;
                case 0:
                    continue;
                default:
                    throw verify_exceptions::ve_crypt("Verifying certificate");
            }
        }
        if (count == 0)
        {
            // no root certificates found
            throw verify_exceptions::ve_crypt("Error loading trusted certificates file");
        }
        // At least one verification failed
        throw verify_exceptions::ve_badcert();
    }

    void X509_certificate::verify_certificate_chain(
        const std::vector<std::string>& intermediate_certs_text,
        const std::vector<root_cert>& root_certs)
    {
        OpenSSL_add_all_digests();

        // Convert intermediate certs to X509*
        std::vector<X509_certificate_impl> intermediate_certs;
        for (const auto& entry : intermediate_certs_text)
        {
            intermediate_certs.emplace_back(entry);
        }
        X509StackWrapper untrusted_certs_stack;
        for (auto& entry : intermediate_certs)
        {
            sk_X509_push(untrusted_certs_stack.GetPtr(), entry.get());
        }

        // Unfortunately, all of our root certificate authorities issued before 2018 are named the same thing:
        // "Sophos Certification Authority". This means we cannot use openssl simple verification
        // because it will just return the first certificate that matches the name, and then fail.
        // We need to just iterate across all root
        // certificates until we find the correct one.
        int lastError = 0;
        std::string lastErrorMessage;

        for (const auto& entry : root_certs)
        {
            try
            {
                if (verify_certificate_chain_impl(*data_, untrusted_certs_stack, entry))
                {
                    return; // matched a root cert
                }
            }
            catch (const verify_exceptions::ve_badcert& ex)
            {
                lastError = ex.getErrorCode();
                PRINT("Bad certificate: " << ex.what());
            }
            catch (const verify_exceptions::ve_crypt& ex)
            {
                lastError = ex.getErrorCode();
                lastErrorMessage = ex.message();
            }
            catch (const std::runtime_error& ex)
            {
            }
        }

        switch (lastError)
        {
            case VerificationTool::SignedFile::bad_certificate:
                throw verify_exceptions::ve_badcert();
            case VerificationTool::SignedFile::openssl_error:
                throw verify_exceptions::ve_crypt(lastErrorMessage);
            default:
                throw verify_exceptions::ve_crypt("Failed to verify certificate chain");
        }

    }

} // namespace crypto
