/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Cipher.h"

#include "Logger.h"

#include "Common/Obfuscation/ICipherException.h"

#include <openssl/conf.h>
#include <openssl/err.h>

#include <cassert>
#include <iostream>
#include <pwd.h>
#include <vector>
// ------------------------------------------------------------------------------------------------

namespace Common
{
    namespace ObfuscationImpl
    {
        EVP_CIPHER_CTX* EvpCipherWrapper::EVP_CIPHER_CTX_new() const { return ::EVP_CIPHER_CTX_new(); }

        void EvpCipherWrapper::EVP_CIPHER_CTX_free(EVP_CIPHER_CTX* a) const { ::EVP_CIPHER_CTX_free(a); }

        int EvpCipherWrapper::EVP_DecryptInit_ex(
            EVP_CIPHER_CTX* ctx,
            const EVP_CIPHER* cipher,
            ENGINE* impl,
            const unsigned char* key,
            const unsigned char* iv) const
        {
            return ::EVP_DecryptInit_ex(ctx, cipher, impl, key, iv);
        }

        int EvpCipherWrapper::EVP_DecryptUpdate(
            EVP_CIPHER_CTX* ctx,
            unsigned char* out,
            int* outl,
            const unsigned char* in,
            int inl) const
        {
            return ::EVP_DecryptUpdate(ctx, out, outl, in, inl);
        }

        int EvpCipherWrapper::EVP_DecryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* outm, int* outl) const
        {
            return ::EVP_DecryptFinal_ex(ctx, outm, outl);
        }

        int EvpCipherWrapper::EVP_EncryptInit_ex(
            EVP_CIPHER_CTX* ctx,
            const EVP_CIPHER* cipher,
            ENGINE* impl,
            const unsigned char* key,
            const unsigned char* iv) const
        {
            return ::EVP_EncryptInit_ex(ctx, cipher, impl, key, iv);
        }

        int EvpCipherWrapper::EVP_EncryptUpdate(
            EVP_CIPHER_CTX* ctx,
            unsigned char* out,
            int* outl,
            const unsigned char* in,
            int inl) const
        {
            return ::EVP_EncryptUpdate(ctx, out, outl, in, inl);
        }

        int EvpCipherWrapper::EVP_EncryptFinal_ex(EVP_CIPHER_CTX* ctx, unsigned char* out, int* outl) const
        {
            return ::EVP_EncryptFinal_ex(ctx, out, outl);
        }

        EvpCipherContext::EvpCipherContext()
        {
            if (!(m_ctxPtr = Common::Obfuscation::evpCipherWrapper()->EVP_CIPHER_CTX_new()))
                handleErrors("Failed to create new EVP Cipher CTX");
        }

        EvpCipherContext::~EvpCipherContext()
        {
            Common::Obfuscation::evpCipherWrapper()->EVP_CIPHER_CTX_free(m_ctxPtr);
        }

        void EvpCipherContext::DecryptInit_ex(
            const EVP_CIPHER* cipher,
            const unsigned char* key,
            const unsigned char* iv)
        {
            if (1 != Common::Obfuscation::evpCipherWrapper()->EVP_DecryptInit_ex(m_ctxPtr, cipher, nullptr, key, iv))
            {
                handleErrors("Failed to initialise EVP Decrypt");
            }
        }

        void EvpCipherContext::DecryptUpdate(unsigned char* out, int* outl, const unsigned char* in, int inl)
        {
            if (1 != Common::Obfuscation::evpCipherWrapper()->EVP_DecryptUpdate(m_ctxPtr, out, outl, in, inl))
            {
                handleErrors("Failed to update EVP Decrypt");
            }
        }

        void EvpCipherContext::DecryptFinal_ex(unsigned char* outm, int* outl)
        {
            if (1 != Common::Obfuscation::evpCipherWrapper()->EVP_DecryptFinal_ex(m_ctxPtr, outm, outl))
            {
                handleErrors("Failed to finalise EVP Decrypt");
            }
        }

        void EvpCipherContext::EncryptInit_ex(
            const EVP_CIPHER* cipher,
            const unsigned char* key,
            const unsigned char* iv)
        {
            if (1 != Common::Obfuscation::evpCipherWrapper()->EVP_EncryptInit_ex(m_ctxPtr, cipher, nullptr, key, iv))
            {
                handleErrors("Failed to initialise EVP Encrypt");
            }
        }

        void EvpCipherContext::EncryptUpdate(unsigned char* out, int* outl, const unsigned char* in, int inl)
        {
            if (1 != Common::Obfuscation::evpCipherWrapper()->EVP_EncryptUpdate(m_ctxPtr, out, outl, in, inl))
            {
                handleErrors("Failed to update EVP Encrypt");
            }
        }

        void EvpCipherContext::EncryptFinal_ex(unsigned char* out, int* outl)
        {
            if (1 != Common::Obfuscation::evpCipherWrapper()->EVP_EncryptFinal_ex(m_ctxPtr, out, outl))
            {
                handleErrors("Failed to finalise EVP Encrypt");
            }
        }

        void EvpCipherContext::handleErrors(const std::string& error)
        {
            LOGDEBUG(ERR_error_string(ERR_get_error(), nullptr));
            throw Common::Obfuscation::ICipherException("SECDeobfuscation Failed due to: " + error);
        }

        ObfuscationImpl::SecureString Cipher::Decrypt(
            const ObfuscationImpl::SecureDynamicBuffer& cipherKey,
            ObfuscationImpl::SecureDynamicBuffer& encrypted)
        {
            if (cipherKey.empty())
            {
                throw Common::Obfuscation::ICipherException("Empty key not allowed");
            }

            if (encrypted.empty())
            {
                throw Common::Obfuscation::ICipherException("String to decrypt is empty");
            }

            const EVP_CIPHER* cipher = EVP_aes_256_cbc();
            assert(EVP_CIPHER_key_length(cipher) > 0);
            const auto REQUIRED_SALT_LENGTH = static_cast<size_t>(EVP_CIPHER_key_length(cipher));

            size_t saltLength = encrypted[0];
            if (saltLength != REQUIRED_SALT_LENGTH)
            {
                throw Common::Obfuscation::ICipherException("Incorrect number of salt bytes");
            }

            if (encrypted.size() < saltLength + 1)
            {
                throw Common::Obfuscation::ICipherException("Salt length is bigger than the input string");
            }
            ObfuscationImpl::SecureDynamicBuffer salt(encrypted.begin() + 1, encrypted.begin() + saltLength + 1);
            ObfuscationImpl::SecureDynamicBuffer cipherText(encrypted.begin() + saltLength + 1, encrypted.end());

            ObfuscationImpl::SecureFixedBuffer<AES256ObfuscationImpl::KeyLength + AES256ObfuscationImpl::IVLength> keyivarray;

            const EVP_MD* digest = EVP_sha512();
            int slen = PKCS5_PBKDF2_HMAC(
                reinterpret_cast<const char*>(cipherKey.data()),
                static_cast<int>(cipherKey.size()),
                salt.data(),
                static_cast<int>(salt.size()),
                AES256ObfuscationImpl::KeyIterations,
                digest,
                keyivarray.size(),
                keyivarray.data());

            if (slen != 1)
            {
                throw Common::Obfuscation::ICipherException("SECDeobfuscation Failed.");
            }

            unsigned char* key = keyivarray.data();
            unsigned char* iv = keyivarray.data() + EVP_CIPHER_key_length(cipher);

            /* Create and initialise the context */
            EvpCipherContext evpCipherWrapper;

            /* Initialise the decryption operation. IMPORTANT - ensure you use a key
             * and IV size appropriate for your cipher
             * In this example we are using 256 bit AES (i.e. a 256 bit key). The
             * IV size for *most* modes is the same as the block size. For AES this
             * is 128 bits */
            evpCipherWrapper.DecryptInit_ex(cipher, key, iv);

            /* Provide the message to be decrypted, and obtain the plaintext output.
             * EVP_DecryptUpdate can be called multiple times if necessary
             */
            Common::ObfuscationImpl::SecureFixedBuffer<Cipher::maxPasswordSize> plaintextbuffer;

            int len = -1;
            evpCipherWrapper.DecryptUpdate(
                plaintextbuffer.data(), &len, (const unsigned char*)cipherText.data(), cipherText.size());
            if (len < 0)
            {
                throw Common::Obfuscation::ICipherException(
                    "SECDeobfuscation failed, EVP_DecryptUpdate did not return a valid length.");
            }

            int plaintext_len = len;

            /* Finalise the decryption. Further plaintext bytes may be written at
             * this stage.
             */
            evpCipherWrapper.DecryptFinal_ex(plaintextbuffer.data() + len, &len);
            plaintext_len += len;

            if (plaintext_len >= Cipher::maxPasswordSize)
            {
                throw Common::Obfuscation::ICipherException(
                    "SECDeobfuscation failed, Decrypted string exceeds maximum length of: " + std::to_string(Cipher::maxPasswordSize));
            }

            Common::ObfuscationImpl::SecureString value(plaintextbuffer.data(), plaintextbuffer.data() + plaintext_len);

            return value;
        }

        std::string Cipher::Encrypt(
            const Common::ObfuscationImpl::SecureDynamicBuffer& cipherKey,
            ObfuscationImpl::SecureDynamicBuffer& salt,
            const std::string& plainPassword)
        {
            if (cipherKey.empty())
            {
                throw Common::Obfuscation::ICipherException("Empty key not allowed");
            }

            if (salt.empty())
            {
                throw Common::Obfuscation::ICipherException("Empty salt not allowed");
            }

            if (plainPassword.empty())
            {
                throw Common::Obfuscation::ICipherException("No password to encrypt");
            }

            const EVP_CIPHER* cipher = EVP_aes_256_cbc();
            assert(EVP_CIPHER_key_length(cipher) > 0);
            const auto REQUIRED_SALT_LENGTH = static_cast<size_t>(EVP_CIPHER_key_length(cipher));

            size_t saltLength = salt.size();
            if (saltLength != REQUIRED_SALT_LENGTH)
            {
                throw Common::Obfuscation::ICipherException("Incorrect number of salt bytes");
            }

            ObfuscationImpl::SecureDynamicBuffer cipherText;

            ObfuscationImpl::SecureFixedBuffer<AES256ObfuscationImpl::KeyLength + AES256ObfuscationImpl::IVLength> keyivarray;

            const EVP_MD* digest = EVP_sha512();
            int slen = PKCS5_PBKDF2_HMAC(
                reinterpret_cast<const char*>(cipherKey.data()),
                static_cast<int>(cipherKey.size()),
                salt.data(),
                static_cast<int>(salt.size()),
                AES256ObfuscationImpl::KeyIterations,
                digest,
                keyivarray.size(),
                keyivarray.data());

            if (slen != 1)
            {
                throw Common::Obfuscation::ICipherException("SECObfuscation Failed.");
            }

            unsigned char* key = keyivarray.data();
            unsigned char* iv = keyivarray.data() + EVP_CIPHER_key_length(cipher);

            /* Create and initialise the context */
            EvpCipherContext evpCipherWrapper;

            /* Initialise the encryption operation. IMPORTANT - ensure you use a key
             * and IV size appropriate for your cipher
             * In this example we are using 256 bit AES (i.e. a 256 bit key). The
             * IV size for *most* modes is the same as the block size. For AES this
             * is 128 bits */
            evpCipherWrapper.EncryptInit_ex(cipher, key, iv);

            /* Provide the message to be encrypted, and obtain the encrypted output.
             * EVP_EncryptUpdate can be called multiple times if necessary
             */
            Common::ObfuscationImpl::SecureFixedBuffer<Cipher::maxPasswordSize + Cipher::AES256ObfuscationImpl::SaltLength> cipherTextBuffer;

            int len = -1;
            evpCipherWrapper.EncryptUpdate(
                cipherTextBuffer.data(), &len, (const unsigned char*)plainPassword.data(), plainPassword.size());
            if (len < 0)
            {
                throw Common::Obfuscation::ICipherException(
                    "SECObfuscation failed, EVP_EncryptUpdate did not return a valid length.");
            }

            int cipherTextLen = len;

            /* Finalise the encryption. Further plaintext bytes may be written at
             * this stage.
             */
            evpCipherWrapper.EncryptFinal_ex(cipherTextBuffer.data() + len, &len);
            cipherTextLen += len;

            if (static_cast<ulong>(cipherTextLen) >= Cipher::maxPasswordSize + Cipher::AES256ObfuscationImpl::SaltLength)
            {
                throw Common::Obfuscation::ICipherException(
                    "SECObfuscation failed, Encrypted string of size: " + std::to_string(cipherTextLen) +
                    " Exceeds maximum length of: " + std::to_string(Cipher::maxPasswordSize + Cipher::AES256ObfuscationImpl::SaltLength));
            }

            std::string value(cipherTextBuffer.data(), cipherTextBuffer.data() + cipherTextLen);

            return value;
        }
    } // namespace ObfuscationImpl
} // namespace Common

Common::ObfuscationImpl::IEvpCipherWrapperPtr& evpCipherWrapperStaticPointer()
{
    static Common::ObfuscationImpl::IEvpCipherWrapperPtr instance =
        Common::ObfuscationImpl::IEvpCipherWrapperPtr(new Common::ObfuscationImpl::EvpCipherWrapper());
    return instance;
}

void Common::ObfuscationImpl::replaceEvpCipherWrapper(IEvpCipherWrapperPtr pointerToReplace)
{
    evpCipherWrapperStaticPointer() = std::move(pointerToReplace);
}

void Common::ObfuscationImpl::restoreEvpCipherWrapper()
{
    evpCipherWrapperStaticPointer() = std::make_unique<Common::ObfuscationImpl::EvpCipherWrapper>();
}

Common::Obfuscation::IEvpCipherWrapper* Common::Obfuscation::evpCipherWrapper()
{
    return evpCipherWrapperStaticPointer().get();
}
