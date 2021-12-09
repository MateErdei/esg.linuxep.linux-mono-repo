/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockEvpCipherWrapper.h"
#include "datatypes/Print.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <pluginimpl/Obfuscation/ICipherException.h>
#include <pluginimpl/ObfuscationImpl/Base64.h>
#include <pluginimpl/ObfuscationImpl/Cipher.h>
#include <pluginimpl/ObfuscationImpl/Obscurity.h>
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

    MockEvpCipherWrapper* m_mockEvpCipherWrapperPtr = nullptr; // Borrowed pointer
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

TEST_F(CipherTest, FailDecryptBecauseSaltLongerThanKey) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer key(5, '*');
    Common::ObfuscationImpl::SecureDynamicBuffer encrypted(200, '*');
    // First byte is treated as the salt length
    encrypted[0] = 150;
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(key, encrypted),
        Common::Obfuscation::ICipherException);
}

namespace
{
    class TestObscurity : public Common::ObfuscationImpl::CObscurity
    {
    public:
        [[nodiscard]] Common::ObfuscationImpl::SecureDynamicBuffer PublicGetPassword() const
        {
            return GetPassword();
        }
    };

    class RealCipherTest : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            Common::ObfuscationImpl::restoreEvpCipherWrapper();
            TestObscurity t;
            m_password = t.PublicGetPassword();
        }
        void TearDown() override
        {
        }

        Common::ObfuscationImpl::SecureDynamicBuffer m_password;
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    };
}


TEST_F(RealCipherTest, SuccessfulDecrypt) // NOLINT
{
    using Common::ObfuscationImpl::SecureDynamicBuffer;
    using Common::ObfuscationImpl::Base64;
    Common::ObfuscationImpl::SecureDynamicBuffer& cipherKey = m_password;

    std::string srcData = "CCD37FNeOPt7oCSNouRhmb9TKqwDvVsqJXbyTn16EHuw6ksTa3NCk56J5RRoVigjd3E=";
    std::string obscuredData = Base64::Decode(srcData);
    SecureDynamicBuffer encrypted(begin(obscuredData) + 1, end(obscuredData));
    auto actualResult = Common::ObfuscationImpl::Cipher::Decrypt(cipherKey, encrypted);
    EXPECT_NE(actualResult.size(), 0);
    EXPECT_EQ(actualResult, "regrABC123pass");
}


TEST_F(RealCipherTest, TestBadSaltSize) // NOLINT
{
    using Common::ObfuscationImpl::SecureDynamicBuffer;
    using Common::ObfuscationImpl::Base64;
    Common::ObfuscationImpl::SecureDynamicBuffer& cipherKey = m_password;

    std::string srcData = "CCD37FNeOPt7oCSNouRhmb9TKqwDvVsqJXbyTn16EHuw6ksTa3NCk56J5RRoVigjd3E=";
    std::string obscuredData = Base64::Decode(srcData);
    obscuredData[1] = 33;
    SecureDynamicBuffer encrypted(begin(obscuredData) + 1, end(obscuredData));
    EXPECT_THROW( // NOLINT
        Common::ObfuscationImpl::Cipher::Decrypt(cipherKey, encrypted),
        Common::Obfuscation::ICipherException);
}
