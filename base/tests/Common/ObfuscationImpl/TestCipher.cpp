/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockEvpCipherWrapper.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Obfuscation/ICipherException.h>
#include <Common/ObfuscationImpl/Cipher.h>
#include <Common/ObfuscationImpl/Obscurity.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class CipherTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        std::unique_ptr<MockEvpCipherWrapper> mockEvpCipherWrapper(new StrictMock<MockEvpCipherWrapper>());
        m_mockEvpCipherWrapperPtr = mockEvpCipherWrapper.get();
        Common::ObfuscationImpl::replaceEvpCipherWrapper(std::move(mockEvpCipherWrapper));
    }
    void TearDown() override { Common::ObfuscationImpl::restoreEvpCipherWrapper(); }

    MockEvpCipherWrapper* m_mockEvpCipherWrapperPtr = nullptr;
    Common::ObfuscationImpl::SecureDynamicBuffer m_password;
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

// ObfuscationImpl::SecureString Cipher::Decrypt(const ObfuscationImpl::SecureDynamicBuffer& cipherKey,
// ObfuscationImpl::SecureDynamicBuffer& encrypted)

TEST_F(CipherTest, BadEncryptionLength) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(31, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer),
        Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, BadPKCS5Password) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer encrypted(33, '*');
    // First byte is treated as the salt length
    encrypted[0] = 32;
    Common::ObfuscationImpl::SecureDynamicBuffer cipherKey; // Empty cipher key
    EXPECT_THROW(                                           // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(cipherKey, encrypted),
        Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailContextConstruction) // NOLINT
{
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(nullptr));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    // Reuse dummyBuffer as both cipherKey and EncryptedText
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer),
        Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailDecryptInit) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_, _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer),
        Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailDecryptUpdate) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptUpdate(_, _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer),
        Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailDecryptFinal) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptUpdate(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptFinal_ex(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer),
        Common::Obfuscation::ICipherException);
}
