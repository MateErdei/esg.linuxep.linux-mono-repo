// Copyright 2019-2023 Sophos Limited. All Rights Reserved.

#include "X509_certificate.h"

#include "crypto_utils.h"
#include "libcryptosupport.h"
#include "verify_exceptions.h"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

using DWORD = uint32_t;

namespace
{
    // Wrapper around these X509 objects take ownership& ensure memory cleanup
    struct X509StoreWrapper
    {
        X509_STORE* m_ref;
        X509StoreWrapper() : m_ref(X509_STORE_new())
        {
        }
        ~X509StoreWrapper()
        {
            X509_STORE_free(m_ref);
        }
        X509_STORE* GetPtr()
        {
            return m_ref;
        }
    };

    struct X509StoreCtxWrapper
    {
        X509_STORE_CTX* m_ref;
        X509StoreCtxWrapper() : m_ref(X509_STORE_CTX_new())
        {
        }
        ~X509StoreCtxWrapper()
        {
            X509_STORE_CTX_free(m_ref);
        }
        X509_STORE_CTX* GetPtr()
        {
            return m_ref;
        }
    };

    struct X509StackWrapper
    {
        STACK_OF(X509) * m_ref;
        X509StackWrapper() : m_ref(sk_X509_new_null()) {};
        ~X509StackWrapper()
        {
            sk_X509_free(m_ref);
        };
        STACK_OF(X509) * GetPtr() const
        {
            return m_ref;
        }
    };
}


namespace crypto
{
    class KeyFreer
    {
        EVP_PKEY* m_Key;

    public:
        explicit KeyFreer(EVP_PKEY* key) : m_Key(key)
        {
        }
        ~KeyFreer()
        {
            if (m_Key)
            {
                EVP_PKEY_free(m_Key);
            }
        }
    };

    class X509_certificate_impl
    {
    public:
        explicit X509_certificate_impl(const std::string& cert_text)
            : cert_(nullptr)
        {
            cert_ = VerificationToolCrypto::X509_decode(cert_text);
            if (!cert_)
                throw crypto::crypto_exception("Bad certificate");
        }
        X509_certificate_impl(const X509_certificate_impl&) = delete;
        X509_certificate_impl(X509_certificate_impl&&) = default;
        X509_certificate_impl& operator=(const X509_certificate_impl&) = delete;
        X509_certificate_impl& operator=(X509_certificate_impl&&) = default;

        ~X509_certificate_impl()
        {
            if (cert_)
            {
                X509_free(cert_);
            }
        }

        X509* cert_;

        [[nodiscard]] X509* get() const
        {
            return cert_;
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

    void X509_certificate::verify_signature(const std::string& body, const manifest::signature& signature)
    {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_VerifyInit(ctx, crypto::construct_digest_algorithm(signature.algo_));
        EVP_VerifyUpdate(ctx, body.c_str(), signature.body_length_);


        // extract public key from certificate
        EVP_PKEY* pubkey = X509_get_pubkey(data_->cert_);
        if (!pubkey)
        {
            throw verify_exceptions::ve_crypt("Getting public key from certificate");
        }
        KeyFreer FreeKey(pubkey);


        int result = EVP_VerifyFinal(ctx, (unsigned char*)(signature.signature_.c_str()), signature.signature_.length(), pubkey);

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

    static bool verify_certificate_chain_impl(
            const X509_certificate_impl& sig_cert,
            const X509StackWrapper& untrusted_certs_stack,
            const root_cert& root_cert
            )
    {
        X509StoreWrapper store;

        int status = 0;

        char* data = const_cast<char*>(root_cert.pem_crt.c_str());
        BIO* in = BIO_new_mem_buf(data, int(root_cert.pem_crt.length()));
        if (!in)
        {
            throw verify_exceptions::ve_crypt("Error opening trusted certificates file");
        }
        int result = 0; // Must be at least one root certificate
        for (;;)
        {
            X509* this_cert = PEM_read_bio_X509(in, NULLPTR, NULLPTR, NULLPTR);
            if (!this_cert)
            {
                break;
            }
            result = X509_STORE_add_cert(store.GetPtr(), this_cert);
            X509_free(this_cert);
            if (!result)
            {
                break;
            }
        }
        BIO_free_all(in);

        if (!result)
        {
            throw verify_exceptions::ve_crypt("Error loading trusted certificates");
        }

        // Verify against this certificate.
        X509StoreCtxWrapper verify_ctx;

        if (X509_STORE_CTX_init(verify_ctx.GetPtr(), store.GetPtr(), sig_cert.get(), untrusted_certs_stack.GetPtr()) != 1)
        {
            throw verify_exceptions::ve_crypt("Error initialising verification context");
        }

        status = X509_verify_cert(verify_ctx.GetPtr());
        switch (status)
        {
            case 1:
                return true;
            case 0:
                throw verify_exceptions::ve_badcert();
            default:
                throw verify_exceptions::ve_crypt("Verifying certificate");
        }
    }

    void X509_certificate::verify_certificate_chain(
        const std::vector<std::string>& intermediate_certs_text,
        const std::vector<root_cert>& root_certs)
    {
        if (intermediate_certs_text.empty())
            throw crypto_exception("Empty chain");

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

        for (const auto& entry : root_certs)
        {
            try
            {
                if (verify_certificate_chain_impl(*data_, untrusted_certs_stack, entry))
                {
                    return; // matched a root cert
                }
            }
            catch (const std::runtime_error& ex)
            {
            }
        }

        throw verify_exceptions::ve_crypt("Failed to verify certificate chain");
    }

} // namespace crypto
