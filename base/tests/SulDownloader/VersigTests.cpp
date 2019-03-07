/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <SulDownloader/suldownloaderdata/IVersig.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

class VersigTests : public ::testing::Test
{
public:
    VersigTests() : m_configurationData(SulDownloader::suldownloaderdata::ConfigurationData::DefaultSophosLocationsURL)
    {
        m_configurationData.setCertificatePath("/installroot/cert");
        rootca = Common::FileSystem::join(m_configurationData.getCertificatePath(), "rootca.crt");
        productDir = "/installroot/cache/update/Primary/product";
        manifestdat = "/installroot/cache/update/Primary/product/manifest.dat";
        versigExec = Common::ApplicationConfiguration::applicationPathManager().getVersigPath();
        fileSystemMock = new MockFileSystem();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(fileSystemMock));
    }
    ~VersigTests() override { Tests::restoreFileSystem(); }
    SulDownloader::suldownloaderdata::ConfigurationData m_configurationData;
    std::string rootca;
    std::string productDir;
    std::string versigExec;
    std::string manifestdat;
    MockFileSystem* fileSystemMock;
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};
using VS = SulDownloader::suldownloaderdata::IVersig::VerifySignature;

TEST_F(VersigTests, verifyReturnsInvalidForInvalidCertificatePath) // NOLINT
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, isFile(rootca)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, verifyReturnsInvalidForInvalidDirectory) // NOLINT
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, isFile(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, returnInvalidIfFailsToFindVersigExecutable) // NOLINT
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, isFile(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, returnInvalidIfNoManitestDatIsFound) // NOLINT
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, isFile(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, passTheCorrectParametersToProcess) // NOLINT
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, isFile(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(true));

    std::string versigExecPath = versigExec;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([versigExecPath]() {
        std::vector<std::string> args;
        args.emplace_back("-c/installroot/cert/rootca.crt");
        args.emplace_back("-f/installroot/cache/update/Primary/product/manifest.dat");
        args.emplace_back("-d/installroot/cache/update/Primary/product");
        args.emplace_back("--silent-off");

        auto mockProcess = new MockProcess();
        EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    ASSERT_EQ(VS::SIGNATURE_VERIFIED, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, signatureFailureIsReportedAsFailure) // NOLINT
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, isFile(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(true));

    std::string versigExecPath = versigExec;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([versigExecPath]() {
        std::vector<std::string> args;
        args.emplace_back("-c/installroot/cert/rootca.crt");
        args.emplace_back("-f/installroot/cache/update/Primary/product/manifest.dat");
        args.emplace_back("-d/installroot/cache/update/Primary/product");
        args.emplace_back("--silent-off");

        auto mockProcess = new MockProcess();
        EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    ASSERT_EQ(VS::SIGNATURE_FAILED, versig->verify(m_configurationData, productDir));
}
