// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "hash_algorithm.h"
#include "hash_exception.h"
#include "libcrypto-compat.h"
#include "verify_exceptions.h"

#include <openssl/x509.h>

#define NULLPTR nullptr

namespace crypto
{
    inline const EVP_MD* construct_digest_algorithm(crypto::hash_algo algo)
    {
        switch (algo)
        {
            case crypto::hash_algo::ALGO_MD5:
                return EVP_md5();
            case crypto::hash_algo::ALGO_SHA1:
                return EVP_sha1();
            case crypto::hash_algo::ALGO_SHA256:
                return EVP_sha256();
            case crypto::hash_algo::ALGO_SHA384:
                return EVP_sha384();
            default:
                // not reached unless we forget to add a case!
                throw crypto::hash_exception("unknown algorithm: " + std::to_string(algo));
        }
    }

    struct BIOWrapper final
    {
        explicit BIOWrapper(const std::string& in)
            : m_bio(NULLPTR)
        {
            m_bio = BIO_new_mem_buf(in.c_str(), int(in.length()));
            if (m_bio == NULLPTR)
            {
                throw verify_exceptions::ve_crypt("Failed to create BIO for string");
            }
        }
        BIOWrapper(BIOWrapper&) = delete;
        ~BIOWrapper()
        {
            reset();
        }
        BIO* m_bio;
        [[nodiscard]] BIO* get() const
        {
            return m_bio;
        }
        [[nodiscard]] bool valid() const
        {
            return m_bio != NULLPTR;
        }
        BIO* release()
        {
            BIO* temp = m_bio;
            m_bio = NULLPTR;
            return temp;
        }
        void reset()
        {
            if (m_bio)
            {
                BIO_free_all(m_bio);
            }
            m_bio = NULLPTR;
        }
    };

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
        [[nodiscard]] X509_STORE* GetPtr() const
        {
            return m_ref;
        }
        [[nodiscard]] X509_STORE* get() const
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
        [[nodiscard]] X509_STORE_CTX* GetPtr() const
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
        [[nodiscard]] STACK_OF(X509) * GetPtr() const
        {
            return m_ref;
        }
    };

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

    using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
    using BIO_MEM_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
}
