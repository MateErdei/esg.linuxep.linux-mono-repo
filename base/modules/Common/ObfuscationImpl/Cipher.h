/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <openssl/evp.h>

//
//	Cipher.h
//
//	Class declaration for encryption/decryption class.

#pragma once

#include "SecureCollection.h"

namespace Common
{
    namespace ObfuscationImpl
    {

        class EvpCipherWrapper
        {
        public:
            EvpCipherWrapper();

            ~EvpCipherWrapper();

            void DecryptInit_ex(const EVP_CIPHER* cipher,
                                const unsigned char* key,
                                const unsigned char* iv);

            void DecryptUpdate(unsigned char* out, int* outl,
                               const unsigned char* in, int inl);

            void DecryptFinal_ex(unsigned char* outm, int* outl);

            void handleErrors();

        private:
            EVP_CIPHER_CTX* m_ctxPtr;
        };


// ________________________________ Cipher __________________________________

// Cipher
//
//
// Encrypts/decrypts the given memory buffer, initialised by a password that is stored in the
// object with protection.

        class Cipher
        {

        public:
            static Common::ObfuscationImpl::SecureString
            Decrypt(const Common::ObfuscationImpl::SecureDynamicBuffer& cipherKey, Common::ObfuscationImpl::SecureDynamicBuffer& encrypted);

        };
    }
}

