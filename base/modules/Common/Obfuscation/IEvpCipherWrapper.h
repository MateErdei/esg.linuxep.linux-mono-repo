/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <openssl/evp.h>

namespace Common
{
    namespace Obfuscation
    {
        class IEvpCipherWrapper
        {
        public:
            virtual ~IEvpCipherWrapper() = default;
            /**
             * Wrapper for openssl EVP_CIPHER_CTX_new
             * @return
             */
            virtual EVP_CIPHER_CTX* EVP_CIPHER_CTX_new() const = 0;
            /**
             * Wrapper for openssl EVP_CIPHER_CTX_free
             * @param a
             */
            virtual void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX* a) const = 0;
            /**
             * Wrapper for openssl EVP_DecryptInit_ex
             * @param ctx
             * @param cipher
             * @param impl
             * @param key
             * @param iv
             * @return
             */
            virtual int EVP_DecryptInit_ex(
                EVP_CIPHER_CTX* ctx,
                const EVP_CIPHER* cipher,
                ENGINE* impl,
                const unsigned char* key,
                const unsigned char* iv) const = 0;
            /**
             * Wrapper for openssl EVP_DecryptUpdate
             * @param ctx
             * @param out
             * @param outl
             * @param in
             * @param inl
             * @return
             */
            virtual int EVP_DecryptUpdate(
                EVP_CIPHER_CTX* ctx,
                unsigned char* out,
                int* outl,
                const unsigned char* in,
                int inl) const = 0;
            /**
             * Wrapper for openssl EVP_DecryptFinal_ex
             * @param ctx
             * @param outm
             * @param outl
             * @return
             */
            virtual int EVP_DecryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* outm, int* outl) const = 0;
        };

        IEvpCipherWrapper* evpCipherWrapper();
    } // namespace Obfuscation
} // namespace Common
