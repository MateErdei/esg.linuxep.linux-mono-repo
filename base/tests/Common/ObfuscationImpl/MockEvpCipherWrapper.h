// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Obfuscation/IEvpCipherWrapper.h"
#include <gmock/gmock.h>
using namespace ::testing;

class MockEvpCipherWrapper : public Common::Obfuscation::IEvpCipherWrapper
{
public:
    MOCK_METHOD(EVP_CIPHER_CTX*, EVP_CIPHER_CTX_new, (), (const override));
    MOCK_METHOD(void, EVP_CIPHER_CTX_free, (EVP_CIPHER_CTX*), (const override));
    MOCK_CONST_METHOD5(
        EVP_DecryptInit_ex,
        int(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*));
    MOCK_CONST_METHOD5(EVP_DecryptUpdate, int(EVP_CIPHER_CTX*, unsigned char*, int*, const unsigned char*, int));
    MOCK_CONST_METHOD3(EVP_DecryptFinal_ex, int(EVP_CIPHER_CTX*, unsigned char*, int*));
    MOCK_CONST_METHOD5(
        EVP_EncryptInit_ex,
        int(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*));
    MOCK_CONST_METHOD5(EVP_EncryptUpdate, int(EVP_CIPHER_CTX*, unsigned char*, int*, const unsigned char*, int));
    MOCK_CONST_METHOD3(EVP_EncryptFinal_ex, int(EVP_CIPHER_CTX*, unsigned char*, int*));
};