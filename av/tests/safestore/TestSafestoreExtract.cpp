// Copyright 2024 Sophos Limited. All rights reserved.

#include "safestore_extract/Extractor.h"
#include "common/ApplicationPaths.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockZipUtils.h"

#include "MockISafeStoreWrapper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace testing;

class TestSafeStoreExtractor : public LogInitializedTests
{
protected:
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/opt/sophos-spl");
        appConfig.setData("PLUGIN_INSTALL", "/opt/sophos-spl/plugins/av");
        m_mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
        mockFileSystem_ = std::make_unique<NaggyMock<MockFileSystem>>();
        mockFilePermissions_ = std::make_unique<NiceMock<MockFilePermissions>>();
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
        Tests::restoreFilePermissions();
        Common::ZipUtilities::restoreZipUtils();
    }

    void setupMockZipUtils(const int& returnVal = 0)
    {
        auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
        EXPECT_CALL(*mockZip, zip(_, _, _,_,_)).WillOnce(Return(returnVal));
        Common::ZipUtilities::replaceZipUtils(std::move(mockZip));
    }

    void setupSuccessFileSystemMock()
    {
        EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
        EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
        EXPECT_CALL(*mockFileSystem_, makedirs(_)).Times(1);

    }

    void setupSuccessFilePermissionsMock()
    {
        EXPECT_CALL(*mockFilePermissions_, chown(_, "root", "root")).Times(2);
        EXPECT_CALL(*mockFilePermissions_, chmod(_, _)).Times(2);

    }

    std::vector<ObjectHandleHolder> setupMockObjectHandle()
    {
        auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
        auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
        void* rawHandle = reinterpret_cast<SafeStoreObjectHandle>(2222);
        ObjectHandleHolder objectHandle{ mockGetIdMethods, mockReleaseMethods };
        *objectHandle.getRawHandlePtr() = rawHandle;
        std::vector<ObjectHandleHolder> searchResults;
        searchResults.push_back(std::move(objectHandle));
        EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle));
        return searchResults;
    }

    std::unique_ptr<NaggyMock<MockFileSystem>> mockFileSystem_;
    std::unique_ptr<NiceMock<MockFilePermissions>> mockFilePermissions_;
    std::unique_ptr<StrictMock<MockISafeStoreWrapper>> m_mockSafeStoreWrapper;
};

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithFilename)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils();

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file","","stuff","/tmp"),0);
}

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithJustSHA)
{
setupSuccessFileSystemMock();
setupSuccessFilePermissionsMock();
Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
Tests::replaceFilePermissions(std::move(mockFilePermissions_));

std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
safestore::SafeStoreWrapper::SafeStoreFilter filter;

EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
.WillOnce(Return(InitReturnCode::OK));
EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
.WillOnce(Return(ByMove(std::move(searchResults))));
EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

setupMockZipUtils();

safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
EXPECT_EQ(e.extract("","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","stuff","/tmp"),0);
}

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithFilenameAndSHA)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils();

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","stuff","/tmp"),0);
}

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithDirectory)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils();

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/","","stuff","/tmp"),0);
}

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithDirectoryAndSHA)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils();

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","stuff","/tmp"),0);
}

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithAbsolutePath)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils();

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/file","","stuff","/tmp"),0);
}

TEST_F(TestSafeStoreExtractor, sucessfullyExtractFromDatabaseWithAbsolutePathAndSHA)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils();

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/file","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f","stuff","/tmp"),0);
}
// filter failures
TEST_F(TestSafeStoreExtractor, extractFailsWithIncorrectSha)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));


    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd08","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, extractFailsWithDirectoryAndIncorrectSha)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));


    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd08","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, extractFailsWithFileAndIncorrectSha)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file";
    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));


    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd08","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, extractFailsWithFullPathAndIncorrectSha)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";
    filter.objectName= "file";
    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return("275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd0f"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));


    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/file","275a021bbfb6489e54d471899f7db9d1663fc695ec2fe2a2c4538aabf651fd08","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, extractFailsWithIncorrectDirectory)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults;
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/stuff";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));


    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/stuff/","","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, extractFailsWithIncorrectFile)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults;
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file2";
    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));



    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file2","","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, extractFailsWithIncorrectFullPath)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults;
    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectLocation= "/tmp";
    filter.objectName= "filedd";
    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));



    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/filedd","","stuff","/tmp"),1);
}
//other failures
TEST_F(TestSafeStoreExtractor, ExtractReturnsFailureWithRelativePath)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };


    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("tmp/file","","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, ExtractReturnsFailureWithInvalidSha)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };


    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/file","@","stuff","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, ExtractReturnsFailureWithMissingPassword)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };


    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();
    safestore::SafeStoreWrapper::SafeStoreFilter filter;

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("/tmp/file","","","/tmp"),1);
}

TEST_F(TestSafeStoreExtractor, ExtractReturnsFailureWhenFailingToExtractFromDatabase)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();

    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(false));



    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file","","stuff","/tmp"),3);
}

TEST_F(TestSafeStoreExtractor, ExtractReturnsFailureWhenZipFails)
{
    setupSuccessFileSystemMock();
    setupSuccessFilePermissionsMock();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    Tests::replaceFilePermissions(std::move(mockFilePermissions_));

    std::vector<ObjectHandleHolder> searchResults = setupMockObjectHandle();

    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_,"SHA256")).WillOnce(Return(""));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectName(_)).WillOnce(Return("file"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_,_)).WillOnce(Return(true));

    setupMockZipUtils(1);

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file","","stuff","/tmp"),3);
}

TEST_F(TestSafeStoreExtractor, extractThrowsWhenNoPasswordFile)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    EXPECT_THROW( safestore::Extractor(std::move(m_mockSafeStoreWrapper)),std::runtime_error);

}

TEST_F(TestSafeStoreExtractor, extractThrowsWhenPasswordFileCannotBeRead)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    EXPECT_THROW( safestore::Extractor(std::move(m_mockSafeStoreWrapper)),std::runtime_error);

}

TEST_F(TestSafeStoreExtractor, extractThrowsWhenCannotInitSafestore)
{
    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::DB_ERROR));
    EXPECT_THROW( safestore::Extractor(std::move(m_mockSafeStoreWrapper)),std::runtime_error);

}

TEST_F(TestSafeStoreExtractor, ExtractReturnsFailureWhenNoThreatInDatabase)
{

    EXPECT_CALL(*mockFileSystem_, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    std::vector<ObjectHandleHolder> searchResults;

    safestore::SafeStoreWrapper::SafeStoreFilter filter;
    filter.objectName= "file";

    EXPECT_CALL(
            *m_mockSafeStoreWrapper,
            initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
            .WillOnce(Return(InitReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(filter))
            .WillOnce(Return(ByMove(std::move(searchResults))));

    safestore::Extractor e = safestore::Extractor(std::move(m_mockSafeStoreWrapper));
    EXPECT_EQ(e.extract("file","","stuff","/tmp"),1);
}