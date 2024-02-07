// Copyright 2018-2024 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "SulDownloader/suldownloaderdata/IVersig.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "Common/Process/IProcessException.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"

namespace
{
    class VersigTests : public ::testing::Test
    {
    public:
        VersigTests()
        {
            m_configurationData.setSophosCDNURLs({});
            m_configurationData.setManifestNames({ "manifest.dat" });
            m_configurationData.setOptionalManifestNames({ "flags_manifest.dat" });
            rootca = "/opt/sophos-spl/base/update/rootcerts";
            productDir = "/opt/sophos-spl/cache/update/Primary/product";
            m_supplementDir = "/opt/sophos-spl/cache/update/Primary/product/supplement";
            manifestdat = "/opt/sophos-spl/cache/update/Primary/product/manifest.dat";
            m_flagsManifest = "/opt/sophos-spl/cache/update/Primary/product/flags_manifest.dat";
            versigExec = Common::ApplicationConfiguration::applicationPathManager().getVersigPath();
            fileSystemMock = new MockFileSystem();
            m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(fileSystemMock));
        }

        void TearDown() override
        {
            Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
        }

        Common::Policy::UpdateSettings m_configurationData;
        std::string rootca;
        std::string productDir;
        std::string m_supplementDir;
        std::string versigExec;
        std::string manifestdat;
        std::string m_flagsManifest;
        MockFileSystem* fileSystemMock;
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
        Tests::ScopedReplaceFileSystem m_replacer;
    };
}
using VS = SulDownloader::suldownloaderdata::IVersig::VerifySignature;

TEST_F(VersigTests, verifyReturnsInvalidForInvalidCertificatePath)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, verifyReturnsInvalidForInvalidDirectory)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, returnInvalidIfFailsToFindVersigExecutable)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(false));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, returnInvalidIfNoManifestDatIsFound)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, isFile(m_flagsManifest)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, listDirectories(productDir)).WillOnce(Return(std::vector<Path>()));

    ASSERT_EQ(VS::INVALID_ARGUMENTS, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, passTheCorrectParametersToProcess)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(m_flagsManifest)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, listDirectories(productDir)).WillOnce(Return(std::vector<Path>()));

    std::string versigExecPath = versigExec;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([versigExecPath]() {
        std::vector<std::string> args;
        args.emplace_back("-c/opt/sophos-spl/base/update/rootcerts");
        args.emplace_back("-f/opt/sophos-spl/cache/update/Primary/product/manifest.dat");
        args.emplace_back("-d/opt/sophos-spl/cache/update/Primary/product");
        args.emplace_back("--silent-off");

        auto mockProcess = new MockProcess();
        EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    ASSERT_EQ(VS::SIGNATURE_VERIFIED, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, passTheCorrectParametersToProcessWithMultipleManifestFiles)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();
    std::vector<Path> supplementPaths = {m_supplementDir};
    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(m_flagsManifest)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, listDirectories(productDir)).WillOnce(Return(supplementPaths));
    EXPECT_CALL(*fileSystemMock, isFile(Common::FileSystem::join(m_supplementDir, "flags_manifest.dat"))).Times(2).WillRepeatedly(Return(true));

    std::string versigExecPath = versigExec;

    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([versigExecPath, &counter]() {
        if (counter++ == 0)
        {
            std::vector<std::string> args;
            args.emplace_back("-c/opt/sophos-spl/base/update/rootcerts");
            args.emplace_back("-f/opt/sophos-spl/cache/update/Primary/product/manifest.dat");
            args.emplace_back("-d/opt/sophos-spl/cache/update/Primary/product");
            args.emplace_back("--silent-off");

            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            std::vector<std::string> args;
            args.emplace_back("-c/opt/sophos-spl/base/update/rootcerts");
            args.emplace_back("-f/opt/sophos-spl/cache/update/Primary/product/supplement/flags_manifest.dat");
            args.emplace_back("-d/opt/sophos-spl/cache/update/Primary/product/supplement");
            args.emplace_back("--silent-off");

            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ASSERT_EQ(VS::SIGNATURE_VERIFIED, versig->verify(m_configurationData, productDir));
}

TEST_F(VersigTests, signatureFailureIsReportedAsFailure)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();

    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(m_flagsManifest)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, listDirectories(productDir)).WillOnce(Return(std::vector<Path>()));

    std::string versigExecPath = versigExec;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([versigExecPath]() {
        std::vector<std::string> args;
        args.emplace_back("-c/opt/sophos-spl/base/update/rootcerts");
        args.emplace_back("-f/opt/sophos-spl/cache/update/Primary/product/manifest.dat");
        args.emplace_back("-d/opt/sophos-spl/cache/update/Primary/product");
        args.emplace_back("--silent-off");

        auto mockProcess = new MockProcess();
        EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(2));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    ASSERT_EQ(VS::SIGNATURE_FAILED, versig->verify(m_configurationData, productDir));
}
TEST_F(VersigTests, willFailOnSingleSignatureFailureWhenProcessingMultipleManifestFiles)
{
    auto versig = SulDownloader::suldownloaderdata::createVersig();
    std::vector<Path> supplementPaths = {m_supplementDir};
    EXPECT_CALL(*fileSystemMock, exists(rootca)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isDirectory(productDir)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isExecutable(versigExec)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(manifestdat)).WillOnce(Return(true));
    EXPECT_CALL(*fileSystemMock, isFile(m_flagsManifest)).WillOnce(Return(false));
    EXPECT_CALL(*fileSystemMock, listDirectories(productDir)).WillOnce(Return(supplementPaths));
    EXPECT_CALL(*fileSystemMock, isFile(Common::FileSystem::join(m_supplementDir, "flags_manifest.dat"))).Times(2).WillRepeatedly(Return(true));

    std::string versigExecPath = versigExec;

    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([versigExecPath, &counter]() {
        if (counter++ == 0)
        {
            std::vector<std::string> args;
            args.emplace_back("-c/opt/sophos-spl/base/update/rootcerts");
            args.emplace_back("-f/opt/sophos-spl/cache/update/Primary/product/manifest.dat");
            args.emplace_back("-d/opt/sophos-spl/cache/update/Primary/product");
            args.emplace_back("--silent-off");

            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            std::vector<std::string> args;
            args.emplace_back("-c/opt/sophos-spl/base/update/rootcerts");
            args.emplace_back("-f/opt/sophos-spl/cache/update/Primary/product/supplement/flags_manifest.dat");
            args.emplace_back("-d/opt/sophos-spl/cache/update/Primary/product/supplement");
            args.emplace_back("--silent-off");

            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(versigExecPath, args)).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Throw(Common::Process::IProcessException("Failed")));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ASSERT_EQ(VS::SIGNATURE_FAILED, versig->verify(m_configurationData, productDir));
}