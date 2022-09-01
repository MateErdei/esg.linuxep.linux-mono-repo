/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//

//	Cipher.h
//
//	Class declaration for encryption/decryption class.
#pragma once

#include "SecureCollection.h"

#include <Common/Obfuscation/IEvpCipherWrapper.h>
#include <openssl/evp.h>

#include <memory>

namespace Common
{
    namespace ObfuscationImpl
    {
        class EvpCipherWrapper : public Common::Obfuscation::IEvpCipherWrapper
        {
            EVP_CIPHER_CTX* EVP_CIPHER_CTX_new() const override;
            void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX* a) const override;
            int EVP_DecryptInit_ex(
                EVP_CIPHER_CTX* ctx,
                const EVP_CIPHER* cipher,
                ENGINE* impl,
                const unsigned char* key,
                const unsigned char* iv) const override;
            int EVP_DecryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl, const unsigned char* in, int inl)
                const override;
            int EVP_DecryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* outm, int* outl) const override;
            int EVP_EncryptInit_ex(
                EVP_CIPHER_CTX* ctx,
                const EVP_CIPHER* cipher,
                ENGINE* impl,
                const unsigned char* key,
                const unsigned char* iv) const override;
            int EVP_EncryptUpdate(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl, const unsigned char* in, int inl)
                const override;
            int EVP_EncryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl) const override;
        };

        /** To be used in tests only */
        using IEvpCipherWrapperPtr = std::unique_ptr<Common::Obfuscation::IEvpCipherWrapper>;
        void replaceEvpCipherWrapper(IEvpCipherWrapperPtr);
        void restoreEvpCipherWrapper();

        class EvpCipherContext
        {
        public:
            EvpCipherContext();

            ~EvpCipherContext();

            void DecryptInit_ex(const EVP_CIPHER* cipher, const unsigned char* key, const unsigned char* iv);

            void DecryptUpdate(unsigned char* out, int* outl, const unsigned char* in, int inl);

            void DecryptFinal_ex(unsigned char* outm, int* outl);

            void EncryptInit_ex(const EVP_CIPHER* cipher, const unsigned char* key, const unsigned char* iv);

            void EncryptUpdate(unsigned char* out, int* outl, const unsigned char* in, int inl);

            void EncryptFinal_ex(unsigned char* out, int* outl);

            void handleErrors(const std::string& error);

        private:
            EVP_CIPHER_CTX* m_ctxPtr;
        };

        // ________________________________ Cipher __________________________________

        // Cipher
        //
        //
        // Encrypts/decrypts the given memory buffer, initialised by a password that is stored in the
        // object with protection.

        namespace Cipher
        {
            Common::ObfuscationImpl::SecureString Decrypt(
                const Common::ObfuscationImpl::SecureDynamicBuffer& cipherKey,
                Common::ObfuscationImpl::SecureDynamicBuffer& encrypted);

            std::string Encrypt(
                const Common::ObfuscationImpl::SecureDynamicBuffer& cipherKey,
                ObfuscationImpl::SecureDynamicBuffer& salt,
                const std::string& plainPassword);

            struct AES256ObfuscationImpl
            {
                static constexpr char AlgorithmByte = 8;
                static constexpr size_t SaltLength = 32;
                static constexpr size_t KeyLength = 256 / 8;
                static constexpr size_t BlockLength = 128 / 8;
                static constexpr size_t IVLength = BlockLength;
                static constexpr size_t KeyIterations = 50000;
            };

            const int maxPasswordSize = 128;
        }
    } // namespace ObfuscationImpl
} // namespace Common
