/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Obfuscation/IEvpCipherWrapper.h>
#include <gmock/gmock.h>
using namespace ::testing;

class MockEvpCipherWrapper : public Common::Obfuscation::IEvpCipherWrapper
{
public:
    MOCK_CONST_METHOD0(EVP_CIPHER_CTX_new, EVP_CIPHER_CTX*(void));
    MOCK_CONST_METHOD1(EVP_CIPHER_CTX_free, void(EVP_CIPHER_CTX*));
    MOCK_CONST_METHOD5(
        EVP_DecryptInit_ex,
        int(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*));
    MOCK_CONST_METHOD5(EVP_DecryptUpdate, int(EVP_CIPHER_CTX*, unsigned char*, int*, const unsigned char*, int));
    MOCK_CONST_METHOD3(EVP_DecryptFinal_ex, int(EVP_CIPHER_CTX*, unsigned char*, int*));
};