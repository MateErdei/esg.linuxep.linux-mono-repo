/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Common/ObfuscationImpl/Cipher.h>
#include <Common/Obfuscation/ICipherException.h>
#include <Common/ObfuscationImpl/Obscurity.h>
#include "MockEvpCipherWrapper.h"

class CipherTest : public ::testing::Test
{
public:

    void SetUp() override
    {
        std::unique_ptr<MockEvpCipherWrapper> mockEvpCipherWrapper (new StrictMock<MockEvpCipherWrapper>());
        m_mockEvpCipherWrapperPtr = mockEvpCipherWrapper.get();
        Common::ObfuscationImpl::replaceEvpCipherWrapper(std::move(mockEvpCipherWrapper));
    }
    void TearDown() override
    {
        Common::ObfuscationImpl::restoreEvpCipherWrapper();
    }

    MockEvpCipherWrapper* m_mockEvpCipherWrapperPtr;
    Common::ObfuscationImpl::SecureDynamicBuffer m_password;
};

// ObfuscationImpl::SecureString Cipher::Decrypt(const ObfuscationImpl::SecureDynamicBuffer& cipherKey, ObfuscationImpl::SecureDynamicBuffer& encrypted)

TEST_F(CipherTest, BadEncryptionLength)
{
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(31,'*');
    EXPECT_THROW(Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer), Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, BadPKCS5Password)
{
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33,'*');
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer2;
    EXPECT_THROW(Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer2, dummyBuffer), Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailContextConstruction)
{
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(nullptr));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33,'*');
    EXPECT_THROW(Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer), Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailDecryptInit)
{
    EVP_CIPHER_CTX * junk = EVP_CIPHER_CTX_new();  //Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_,_,_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33,'*');
    EXPECT_THROW(Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer), Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailDecryptUpdate)
{
    EVP_CIPHER_CTX * junk = EVP_CIPHER_CTX_new();  //Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_,_,_,_,_)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptUpdate(_,_,_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33,'*');
    EXPECT_THROW(Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer), Common::Obfuscation::ICipherException);
}

TEST_F(CipherTest, FailDecryptFinal)
{
    EVP_CIPHER_CTX * junk = EVP_CIPHER_CTX_new();  //Junk is freed by the EvpCipherContext object
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_new()).WillOnce(Return(junk));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptInit_ex(_,_,_,_,_)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptUpdate(_,_,_,_,_)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_DecryptFinal_ex(_,_,_)).WillOnce(Return(0));
    EXPECT_CALL(*m_mockEvpCipherWrapperPtr, EVP_CIPHER_CTX_free(junk)).WillOnce(Invoke(EVP_CIPHER_CTX_free));
    Common::ObfuscationImpl::SecureDynamicBuffer dummyBuffer(33,'*');
    EXPECT_THROW(Common::ObfuscationImpl::Cipher::Decrypt(dummyBuffer, dummyBuffer), Common::Obfuscation::ICipherException);
}




