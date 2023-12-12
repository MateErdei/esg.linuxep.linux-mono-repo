// Copyright 2023 Sophos Limited. All rights reserved.

#include "DownloadReportMatchers.h"
#include "JwtUtils.h"

#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/Process/IProcess.h"
#include "SulDownloader/SulDownloader.h"
#include "SulDownloader/sdds3/SDDS3Repository.h"
#include "SulDownloader/sdds3/Sdds3RepositoryFactory.h"
#include "SulDownloader/sdds3/SusRequester.h"
#include "SulDownloader/suldownloaderdata/VersigImpl.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/SulDownloader/MockSdds3Wrapper.h"
#include "tests/SulDownloader/MockSignatureVerifierWrapper.h"
#include "tests/SulDownloader/MockVersig.h"
#include "tests/SulDownloader/Sdds3ReplaceAndRestore.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;

class TestDownloadReports : public LogInitializedTests
{
protected:
    ~TestDownloadReports() override
    {
        Tests::restoreFileSystem();
        SulDownloader::Sdds3RepositoryFactory::instance().restoreCreator();
        Tests::restoreSdds3Wrapper();
        SulDownloader::suldownloaderdata::VersigFactory::instance().restoreCreator();
        Common::Process::restoreCreator();
    }

    static void replaceSdds3RepositoryCreator(RespositoryCreaterFunc creatorMethod)
    {
        // This cannot be invoked directly in tests because each test is a subclass of this, and they don't inherit
        // being a friend of Sdds3RepositoryFactory
        SulDownloader::Sdds3RepositoryFactory::instance().replaceCreator(std::move(creatorMethod));
    }
};

TEST_F(TestDownloadReports, VersionFromVersionIniIsUsedInLogAndInDownloadReport)
{
    MemoryAppenderHolder memoryAppenderHolderSuldownloaderdata{ "suldownloaderdata" };
    UsingMemoryAppender usingMemoryAppenderSuldownloaderdata{ memoryAppenderHolderSuldownloaderdata };

    auto mockFileSystem = std::make_unique<MockFileSystem>();
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    ON_CALL(
        *mockFileSystem,
        exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh"))
        .WillByDefault(Return(true));
    ON_CALL(
        *mockFileSystem,
        isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillByDefault(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3" }));
    ON_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini")).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3" }));
    EXPECT_CALL(*mockFileSystem, writeFile).Times(AnyNumber());
    EXPECT_CALL(
        *mockFileSystem,
        writeFile(
            StartsWith("/opt/sophos-spl/tmp/output"),
            HasSingleComponentWithWarehouseAndInstalledVersion("1.2.3", "1.2.3")));

    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    susResponse.body = R"({"suites": ["sdds3.ServerProtectionLinux-Base"], "release-groups": []})";
    susResponse.headers["X-Content-Signature"] = generateJwt(susResponse.body);
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    ON_CALL(*mockHttpRequester, post).WillByDefault(Return(susResponse));

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
        [&mockHttpRequester, &mockSignatureVerifierWrapper]()
        {
            auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    sophlib::sdds3::PackageRef packageRef;
    packageRef.version_ = "1.2.4";
    packageRef.features_.emplace_back("CORE");
    packageRef.lineId_ = "ServerProtectionLinux-Base-component";
    packageRef.name_ = "base";
    std::vector<sophlib::sdds3::PackageRef> packageRefs{ packageRef };
    EXPECT_CALL(*mockSdds3Wrapper, getPackages).WillRepeatedly(Return(packageRefs));
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);

    EXPECT_TRUE(memoryAppenderHolderSuldownloaderdata.appenderContains(
        "INFO - Installing product: ServerProtectionLinux-Base-component version: 1.2.3"));
}

TEST_F(TestDownloadReports, WhenVersionIniVersionAndSddsVersionMismatchDownloadReportHasComponentRigidNameAndProductName)
{
    MemoryAppenderHolder memoryAppenderHolderSuldownloaderdata{ "suldownloaderdata" };
    UsingMemoryAppender usingMemoryAppenderSuldownloaderdata{ memoryAppenderHolderSuldownloaderdata };

    auto mockFileSystem = std::make_unique<MockFileSystem>();
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    ON_CALL(
            *mockFileSystem,
            exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh"))
            .WillByDefault(Return(true));
    ON_CALL(
            *mockFileSystem,
            isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
            .WillByDefault(Return(true));
    EXPECT_CALL(
            *mockFileSystem,
            readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
            .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3" }));
    ON_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini")).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines("/opt/sophos-spl/base/VERSION.ini"))
            .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3" }));
    EXPECT_CALL(*mockFileSystem, writeFile).Times(AnyNumber());
    EXPECT_CALL(
            *mockFileSystem,
            writeFile(
                    StartsWith("/opt/sophos-spl/tmp/output"),
                    HasSingleComponent("ServerProtectionLinux-Base-component", "base", "1.2.3", "1.2.3")));

    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    susResponse.body = R"({"suites": ["sdds3.ServerProtectionLinux-Base"], "release-groups": []})";
    susResponse.headers["X-Content-Signature"] = generateJwt(susResponse.body);
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    ON_CALL(*mockHttpRequester, post).WillByDefault(Return(susResponse));

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
            [&mockHttpRequester, &mockSignatureVerifierWrapper]()
            {
                auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
                return std::make_unique<SDDS3Repository>(std::move(susRequester));
            });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    sophlib::sdds3::PackageRef packageRef;
    packageRef.version_ = "1.2.4";
    packageRef.features_.emplace_back("CORE");
    packageRef.lineId_ = "ServerProtectionLinux-Base-component";
    packageRef.name_ = "base";
    std::vector<sophlib::sdds3::PackageRef> packageRefs{ packageRef };
    EXPECT_CALL(*mockSdds3Wrapper, getPackages).WillRepeatedly(Return(packageRefs));
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);
}

TEST_F(TestDownloadReports, VersionFromSddsIsNotLogged)
{
    MemoryAppenderHolder memoryAppenderHolderSulDownloader{ "suldownloader" };
    UsingMemoryAppender usingMemoryAppenderSulDownloader{ memoryAppenderHolderSulDownloader };
    MemoryAppenderHolder memoryAppenderHolderSdds3{ "SulDownloaderSDDS3" };
    UsingMemoryAppender usingMemoryAppenderSdds3{ memoryAppenderHolderSdds3 };
    MemoryAppenderHolder memoryAppenderHolderSuldownloaderdata{ "suldownloaderdata" };
    UsingMemoryAppender usingMemoryAppenderSuldownloaderdata{ memoryAppenderHolderSuldownloaderdata };

    auto mockFileSystem = std::make_unique<MockFileSystem>();
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    ON_CALL(
        *mockFileSystem,
        exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh"))
        .WillByDefault(Return(true));
    ON_CALL(
        *mockFileSystem,
        isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillByDefault(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3" }));
    ON_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini")).WillByDefault(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.2.3" }));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    susResponse.body = R"({"suites": ["sdds3.ServerProtectionLinux-Base"], "release-groups": []})";
    susResponse.headers["X-Content-Signature"] = generateJwt(susResponse.body);
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    ON_CALL(*mockHttpRequester, post).WillByDefault(Return(susResponse));

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
        [&mockHttpRequester, &mockSignatureVerifierWrapper]()
        {
            auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    sophlib::sdds3::PackageRef packageRef;
    packageRef.version_ = "1.2.4";
    packageRef.features_.emplace_back("CORE");
    packageRef.lineId_ = "ServerProtectionLinux-Base-component";
    packageRef.name_ = "base";
    std::vector<sophlib::sdds3::PackageRef> packageRefs{ packageRef };
    EXPECT_CALL(*mockSdds3Wrapper, getPackages).WillRepeatedly(Return(packageRefs));
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    const auto result =
        SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);

    // Make sure it finished, and hence logged everything it would
    EXPECT_EQ(result, 0);

    EXPECT_FALSE(memoryAppenderHolderSuldownloaderdata.appenderContains("1.2.4"));
    EXPECT_FALSE(memoryAppenderHolderSdds3.appenderContains("1.2.4"));
    EXPECT_FALSE(memoryAppenderHolderSulDownloader.appenderContains("1.2.4"));
}

TEST_F(
    TestDownloadReports,
    VersionIniMissingWillUseSddsVersionInLogAndWarehouseVersionInDownloadReportButAlsoBlankInstalledVersion)
{
    MemoryAppenderHolder memoryAppenderHolderSulDownloader{ "suldownloader" };
    UsingMemoryAppender usingMemoryAppenderSulDownloader{ memoryAppenderHolderSulDownloader };
    MemoryAppenderHolder memoryAppenderHolderSdds3{ "SulDownloaderSDDS3" };
    UsingMemoryAppender usingMemoryAppenderSdds3{ memoryAppenderHolderSdds3 };
    MemoryAppenderHolder memoryAppenderHolderSuldownloaderdata{ "suldownloaderdata" };
    UsingMemoryAppender usingMemoryAppenderSuldownloaderdata{ memoryAppenderHolderSuldownloaderdata };

    auto mockFileSystem = std::make_unique<MockFileSystem>();
    ON_CALL(*mockFileSystem, isDirectory("output_dir/output")).WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isDirectory("output_dir")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, readFile("input")).WillByDefault(Return(R"({
 "sophosCdnURLs": [
  "https://sdds3.sophosupd.com",
  "https://sdds3.sophosupd.net"
 ],
 "JWToken": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6IjkyZmU3N2M2LWIxNjctNGE0Zi04MGI0LWZiZjFkMDcwZjk1YiJ9.eyJkZXZpY2VJZCI6IjlmOGE2M2VlLTc4YjktNDVlZi1hOTNiLTQ1Y2MzZjc5YmUzNiIsInRlbmFudElkIjoiYzY1NTc2Y2ItMDEyYS00YjYzLWFjNTgtOWNkNTk4YTBjMWRmIiwiZmVhdHVyZUNvZGUiOlsiZl9zcnZfbGl2ZXF1ZXJ5IiwiZl9zcnZfbWwiLCJmX3Nydl9zY2hlZHVsZWRxdWVyeSIsImZfc3J2X2F2IiwiZl9zcnZfc3RhYyIsImZfc3J2X21kciIsImZfZW5kcG9pbnRfdXBkYXRpbmciXSwiZGV2aWNlVHlwZSI6InNlcnZlciIsInBsYXRmb3JtIjoicG9zaXgiLCJpYXQiOjE3MDA3NDM1MzAsImV4cCI6MTcwMDgyOTkzMCwiYXVkIjoiaHR0cDovL2Rpcy5jZW50cmFsLnNvcGhvcy5jb20iLCJpc3MiOiJpZHAuZGlzLmNlbnRyYWwuc29waG9zLmNvbSJ9.b-XzUk0Hssj8Tv1RtYeiQVD8pSZePJx9sHzyzIe_YgmGBrRB4Anu2dcABcKYrEAIU_I7ou-kaXQG4QSSZL01w-pFNWZ1ydUP7ZvUMi4SC_p7Ioezcs2dDDQn0C4t7GroRyWYYrGz0W9J3wyRT4Qg5_HCIsH8istvhXomxXQGSAfEuKqsvVfrlG9XRY7HZIQl_FgHypzr7S2dHRNyYSOuO79Ms_S-OqRlZO43Wa9NQ1TNEwzSHXjZRVZhl3N7fxgVHvkPJJfrabVPLFW3N4cmF0y1OUiLdyMIet5sEXvIjpJ_miL0ih8OMhpgSjDEqInfF3dwKexTf7CQaJPPDBF3Sg",
 "tenantId": "c65576cb-012a-4b63-ac58-9cd598a0c1df",
 "deviceId": "9f8a63ee-78b9-45ef-a93b-45cc3f79be36",
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "features": ["CORE"],
 "sophosSusURL": "https://sus.sophosupd.com",
}
)"));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
    ON_CALL(*mockFileSystem, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
    ON_CALL(
        *mockFileSystem,
        exists("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh"))
        .WillByDefault(Return(true));
    ON_CALL(
        *mockFileSystem,
        isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillByDefault(Return(false));
    ON_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini")).WillByDefault(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile).Times(AnyNumber());
    EXPECT_CALL(
        *mockFileSystem,
        writeFile(
            StartsWith("/opt/sophos-spl/tmp/output"), HasSingleComponentWithWarehouseAndInstalledVersion("1.2.4", "")));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    auto mockHttpRequester = std::make_shared<MockHTTPRequester>();
    Common::HttpRequests::Response susResponse;
    susResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    susResponse.body = R"({"suites": ["sdds3.ServerProtectionLinux-Base"], "release-groups": []})";
    susResponse.headers["X-Content-Signature"] = generateJwt(susResponse.body);
    susResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    ON_CALL(*mockHttpRequester, post).WillByDefault(Return(susResponse));

    auto mockSignatureVerifierWrapper = std::make_shared<MockSignatureVerifierWrapper>();

    replaceSdds3RepositoryCreator(
        [&mockHttpRequester, &mockSignatureVerifierWrapper]()
        {
            auto susRequester = std::make_unique<SDDS3::SusRequester>(mockHttpRequester, mockSignatureVerifierWrapper);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        });

    auto mockSdds3Wrapper = std::make_unique<MockSdds3Wrapper>();
    sophlib::sdds3::PackageRef packageRef;
    packageRef.version_ = "1.2.4";
    packageRef.features_.emplace_back("CORE");
    packageRef.lineId_ = "ServerProtectionLinux-Base-component";
    packageRef.name_ = "base";
    std::vector<sophlib::sdds3::PackageRef> packageRefs{ packageRef };
    EXPECT_CALL(*mockSdds3Wrapper, getPackages).WillRepeatedly(Return(packageRefs));
    Tests::replaceSdds3Wrapper(std::move(mockSdds3Wrapper));

    auto mockVersig = std::make_unique<MockVersig>();
    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&mockVersig]()
                                                                               { return std::move(mockVersig); });

    Common::Process::replaceCreator([]() { return std::make_unique<MockProcess>(); });

    SulDownloader::fileEntriesAndRunDownloader("input", "output_dir/output", "supplement_only.marker", 0s);

    EXPECT_TRUE(memoryAppenderHolderSdds3.appenderContains(
        "WARN - Failed to read VERSION.ini from download cache for: 'ServerProtectionLinux-Base-component', falling back to SDDS version 1.2.4"));
    EXPECT_TRUE(memoryAppenderHolderSuldownloaderdata.appenderContains(
        "INFO - Installing product: ServerProtectionLinux-Base-component version: 1.2.4"));
}
