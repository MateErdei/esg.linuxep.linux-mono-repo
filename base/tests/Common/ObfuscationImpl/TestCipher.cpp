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

TEST_F(CipherTest, DecryptThrowsIfSaltLengthIsWrong) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(32, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 31;
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "Incorrect number of salt bytes");
    }
}

TEST_F(CipherTest, DecryptThrowsWithEmptyKey) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer encrypted(33, '*');
    // First byte is treated as the salt length
    encrypted[0] = 32;
    Common::ObfuscationImpl::SecureDynamicBuffer cipherKey; // Empty cipher key
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(cipherKey, encrypted);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "Empty key not allowed");
    }
}

TEST_F(CipherTest, DecryptThrowsWithEmptyEncryptedString) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer cipherKey(33, '*');
    // First byte is treated as the salt length
    cipherKey[0] = 32;
    Common::ObfuscationImpl::SecureDynamicBuffer encrypted; // Empty cipher key
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(cipherKey, encrypted);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "String to decrypt is empty");
    }
}

TEST_F(CipherTest, FailContextConstruction) // NOLINT
{
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(nullptr));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    // Reuse dummyBuffer as both cipherKey and EncryptedText
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to create new EVP Cipher CTX");
    }
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
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to initialise EVP Decrypt");
    }
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
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to update EVP Decrypt");
    }
}

TEST_F(CipherTest, FailDecryptFinal) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptUpdate(_, _, _, _, _)).WillOnce(DoAll(SetArgPointee<2>(60), Return(1)));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptFinal_ex(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to finalise EVP Decrypt");
    }
}

TEST_F(CipherTest, FailDecryptOversizedPassword) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptUpdate(_, _, _, _, _)).WillOnce(DoAll(SetArgPointee<2>(128), Return(1)));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptFinal_ex(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation failed, Decrypted string exceeds maximum length of: 128");
    }
}

TEST_F(CipherTest, FailDecryptBecauseSaltLongerThanKey) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer key(5, '*');
    Common::ObfuscationImpl::SecureDynamicBuffer encrypted(200, '*');
    // First byte is treated as the salt length
    encrypted[0] = 150;
    try
    {
        Common::ObfuscationImpl::Cipher::Decrypt(key, encrypted);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "Incorrect number of salt bytes");
    }
}

TEST_F(CipherTest, EncryptWithoutKeyThrows) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer keyBuffer;
    Common::ObfuscationImpl::SecureDynamicBuffer saltBuffer(32, '*');
    std::string dummyPassword = "password";
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(keyBuffer, saltBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "Empty key not allowed");
    }
}

TEST_F(CipherTest, EncryptWithoutSaltThrows) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer keyBuffer(32, '*');
    Common::ObfuscationImpl::SecureDynamicBuffer saltBuffer;
    std::string dummyPassword = "password";
    // First byte is treated as the salt length
    keyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(keyBuffer, saltBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "Empty salt not allowed");
    }
}

TEST_F(CipherTest, EncryptWithoutPasswordThrows) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer keyBuffer(32, '*');
    Common::ObfuscationImpl::SecureDynamicBuffer saltBuffer(32, '*');
    std::string dummyPassword;
    // First byte is treated as the salt length
    keyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(keyBuffer, saltBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "No password to encrypt");
    }
}

TEST_F(CipherTest, BadEncryptSaltLength) // NOLINT
{
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(31, '*');
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    std::string dummyPassword = "password";
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(dummyBuffer, dummyBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "Incorrect number of salt bytes");
    }
}

TEST_F(CipherTest, FailEncryptInit) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptInit_ex(_, _, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(32, '*');
    std::string dummyPassword = "password";
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(dummyBuffer, dummyBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to initialise EVP Encrypt");
    }
}

TEST_F(CipherTest, FailEncryptUpdate) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptUpdate(_, _, _, _, _)).WillOnce(DoAll(SetArgPointee<2>(60), Return(0)));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(32, '*');
    std::string dummyPassword = "password";
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(dummyBuffer, dummyBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to update EVP Encrypt");
    }
}

TEST_F(CipherTest, FailEncryptFinal) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptUpdate(_, _, _, _, _)).WillOnce(DoAll(SetArgPointee<2>(60), Return(1)));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptFinal_ex(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(32, '*');
    std::string dummyPassword = "password";
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(dummyBuffer, dummyBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECDeobfuscation Failed due to: Failed to finalise EVP Encrypt");
    }
}

TEST_F(CipherTest, FailEncryptOversizedPassword) // NOLINT
{
    EVP_CIPHER_CTX* junk = EVP_CIPHER_CTX_new(); // Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptInit_ex(_, _, _, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptUpdate(_, _, _, _, _)).WillOnce(DoAll(SetArgPointee<2>(128), Return(1)));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_EncryptFinal_ex(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(32, '*');
    std::string dummyPassword = "password";
    // First byte is treated as the salt length
    dummyBuffer[0] = 32;
    try
    {
        Common::ObfuscationImpl::Cipher::Encrypt(dummyBuffer, dummyBuffer, dummyPassword);
        FAIL(); // Should not be allowed to get to here.
    }
    catch (const Common::Obfuscation::ICipherException& exception)
    {
        ASSERT_EQ(std::string(exception.what()), "SECObfuscation failed, Encrypted string of size: 256 Exceeds maximum length of: 160");
    }
}
