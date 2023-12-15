// Copyright 2023 Sophos Limited. All rights reserved.

#include "MockSdds3Wrapper.h"
#include "MockSusRequester.h"
#include "Sdds3ReplaceAndRestore.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "SulDownloader/sdds3/ISdds3Wrapper.h"
#include "SulDownloader/sdds3/SDDS3CacheUtils.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>


class SDDS3CacheUtilsTest : public MemoryAppenderUsingTests
{
public:
    MockSdds3Wrapper& setupSdds3WrapperAndGetMock()
    {
        auto* sdds3WrapperMock = new StrictMock<MockSdds3Wrapper>();

        auto* pointer = sdds3WrapperMock;
        replacer_.replace(std::unique_ptr<SulDownloader::ISdds3Wrapper>(sdds3WrapperMock));
        return *pointer;
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

protected:
    SDDS3CacheUtilsTest() : MemoryAppenderUsingTests("SulDownloaderSDDS3"), usingMemoryAppender_(*this) {}

    Tests::ScopedReplaceSdds3Wrapper replacer_;
    std::unique_ptr<MockFileSystem> mockFileSystem_ = std::make_unique<MockFileSystem>();

    UsingMemoryAppender usingMemoryAppender_;
};

TEST_F(SDDS3CacheUtilsTest, testCheckCacheSucessCase)
{

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(_)).Times(2).WillRepeatedly(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base">
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="LocalRepData" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
  </package-ref>
</suite>)";
    std::string supplementXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<supplement name="LocalRepData" timestamp="2023-10-25T22:55:50Z">
  <package-ref src="LocalRepData_2023.10.25.21.50.55.1.955eb5bcf6.zip" size="313172" sha256="">
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="0" />
        <release-group name="1" />
        <release-group name="A" />
        <release-group name="B" />
        <release-group name="C" />
      </tag>
    </tags>
  </package-ref>
</supplement>
)";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob( _))
            .Times(2)
            .WillOnce(Return(baseSuiteXML))
            .WillOnce(Return(supplementXML));
    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_TRUE(sdds3CacheUtils.checkCache({"ServerProtectionLinux-Base"},{"C"}));

}

TEST_F(SDDS3CacheUtilsTest, testMultipleSupplements)
{

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(_)).Times(3).WillRepeatedly(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base">
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="LocalRepData" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
    <supplement-ref src="LocalRepData2" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
  </package-ref>
</suite>)";
    std::string supplementXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<supplement name="LocalRepData" timestamp="2023-10-25T22:55:50Z">
  <package-ref src="LocalRepData_2023.10.25.21.50.55.1.955eb5bcf6.zip" size="313172" sha256="">
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="0" />
        <release-group name="1" />
        <release-group name="A" />
        <release-group name="B" />
        <release-group name="C" />
      </tag>
    </tags>
  </package-ref>
</supplement>
)";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob( _))
            .Times(3)
            .WillOnce(Return(baseSuiteXML))
            .WillOnce(Return(supplementXML))
            .WillOnce(Return(supplementXML));
    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_TRUE(sdds3CacheUtils.checkCache({"ServerProtectionLinux-Base"},{"C"}));

}

TEST_F(SDDS3CacheUtilsTest, testMultiplePackagesWithSupplements)
{

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(_)).Times(3).WillRepeatedly(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base">
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="LocalRepData" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
  </package-ref>
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="LocalRepData2" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
  </package-ref>
</suite>)";
    std::string supplementXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<supplement name="LocalRepData" timestamp="2023-10-25T22:55:50Z">
  <package-ref src="LocalRepData_2023.10.25.21.50.55.1.955eb5bcf6.zip" size="313172" sha256="">
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="0" />
        <release-group name="1" />
        <release-group name="A" />
        <release-group name="B" />
        <release-group name="C" />
      </tag>
    </tags>
  </package-ref>
</supplement>
)";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob( _))
            .Times(3)
            .WillOnce(Return(baseSuiteXML))
            .WillOnce(Return(supplementXML))
            .WillOnce(Return(supplementXML));
    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_TRUE(sdds3CacheUtils.checkCache({"ServerProtectionLinux-Base"},{"C"}));

}

TEST_F(SDDS3CacheUtilsTest, testMultiplePackagesInSupplements)
{

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(_)).Times(2).WillRepeatedly(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base">
  <package-ref src="SPL-Anti-Virus-Plugin_1.1.3.0.70.fe47686cc5.zip" size="37078436" sha256="0a5d336f77af79f63268f0d6f36842469349092261781d1c4d3115605fc72646">
    <name>SPL-Anti-Virus-Plugin</name>
    <version>1.1.3.0.70</version>
    <line-id>ServerProtectionLinux-Plugin-AV</line-id>
    <nonce>fe47686cc5</nonce>
    <description>SPL-Anti-Virus-Plugin</description>
    <decode-path>ServerProtectionLinux-Plugin-AV</decode-path>
    <features>
      <feature name="AV" />
    </features>
    <platforms>
      <platform name="LINUX_ARM64" />
    </platforms>
    <supplement-ref src="DataSetA" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/plugins/av/chroot/susi/update_source/vdl" />
  </package-ref>
  <package-ref src="SPL-Anti-Virus-Plugin_1.1.3.1.72.f18af7623e.zip" size="39340909" sha256="a3b94ea101ee1265df79f297fdda73e2a44a4a3ef1d945127d437104a1e24d13">
    <name>SPL-Anti-Virus-Plugin</name>
    <version>1.1.3.1.72</version>
    <line-id>ServerProtectionLinux-Plugin-AV</line-id>
    <nonce>f18af7623e</nonce>
    <description>SPL-Anti-Virus-Plugin</description>
    <decode-path>ServerProtectionLinux-Plugin-AV</decode-path>
    <features>
      <feature name="AV" />
    </features>
    <platforms>
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="DataSetA" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/plugins/av/chroot/susi/update_source/vdl" />
  </package-ref>
</suite>)";
    std::string supplementXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<supplement name="DataSetA" timestamp="2023-12-12T15:07:43Z">
  <package-ref src="DataSetA_2023.12.10.20.32.22.1.9567dc4020.zip" size="18738252" sha256="e481c9ff9991576470f979da9c6b75cc2015ded62b9db3a36760921f461c6de2">
    <name>DataSetA</name>
    <version>2023.12.10.20.32.22.1</version>
    <line-id>DataSetA</line-id>
    <nonce>9567dc4020</nonce>
    <description>timestamp=2023-12-10T20:32:26Z manifest_md5=cafb8a220bea7a1b8f60bef08ddf8925</description>
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="GranularE" />
        <release-group name="GranularF" />
      </tag>
    </tags>
  </package-ref>
  <package-ref src="DataSetA_2023.12.10.20.33.31.2.9567dc4020.zip" size="18738252" sha256="d0703bedef5f7815a2ca23ea2f8a616b9e13c7415ce99d02cdf08c86b408abfe">
    <name>DataSetA</name>
    <version>2023.12.10.20.33.31.2</version>
    <line-id>DataSetA</line-id>
    <nonce>9567dc4020</nonce>
    <description>timestamp=2023-12-10T20:33:35Z manifest_md5=cafb8a220bea7a1b8f60bef08ddf8925</description>
    <tags>
      <tag name="NEXT">
        <release-group name="GranularE" />
        <release-group name="GranularF" />
      </tag>
    </tags>
  </package-ref>
  <package-ref src="DataSetA_2023.12.12.4.2.32.1.de51b5afae.zip" size="18751500" sha256="b28e81b9c6ce21611760f41c47803bcad4315e5be2561c8d89db6d65c7706ee7">
    <name>DataSetA</name>
    <version>2023.12.12.4.2.32.1</version>
    <line-id>DataSetA</line-id>
    <nonce>de51b5afae</nonce>
    <description>timestamp=2023-12-12T04:02:39Z manifest_md5=c925ae48a63662dfe02551cdcd39cff8</description>
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="GranularC" />
        <release-group name="GranularD" />
      </tag>
    </tags>
  </package-ref>
  <package-ref src="DataSetA_2023.12.12.4.3.39.2.de51b5afae.zip" size="18751500" sha256="48379b43ff3540ccbf6eeddde12410b6461f12b56e90234932a2eb59cc7cdf19">
    <name>DataSetA</name>
    <version>2023.12.12.4.3.39.2</version>
    <line-id>DataSetA</line-id>
    <nonce>de51b5afae</nonce>
    <description>timestamp=2023-12-12T04:03:46Z manifest_md5=c925ae48a63662dfe02551cdcd39cff8</description>
    <tags>
      <tag name="NEXT">
        <release-group name="GranularC" />
        <release-group name="GranularD" />
      </tag>
    </tags>
  </package-ref>
  <package-ref src="DataSetA_2023.12.12.11.27.33.1.2dcdaf7483.zip" size="18755657" sha256="a6cf11b5a73d42c3b295a3cc70085c5bd4b383764322196785557b689b01554f">
    <name>DataSetA</name>
    <version>2023.12.12.11.27.33.1</version>
    <line-id>DataSetA</line-id>
    <nonce>2dcdaf7483</nonce>
    <description>timestamp=2023-12-12T11:27:38Z manifest_md5=3938bebb34dfa90741fbe10e0b3cf83f</description>
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="GranularInitial" />
        <release-group name="GranularDogFood" />
        <release-group name="GranularA" />
        <release-group name="GranularB" />
      </tag>
    </tags>
  </package-ref>
  <package-ref src="DataSetA_2023.12.12.11.28.36.2.2dcdaf7483.zip" size="18755657" sha256="3290ce2c5bd67c5ed3bd1157cedf4e17de3e2e0c9afd531615cbc4e3fc10409c">
    <name>DataSetA</name>
    <version>2023.12.12.11.28.36.2</version>
    <line-id>DataSetA</line-id>
    <nonce>2dcdaf7483</nonce>
    <description>timestamp=2023-12-12T11:28:41Z manifest_md5=3938bebb34dfa90741fbe10e0b3cf83f</description>
    <tags>
      <tag name="NEXT">
        <release-group name="GranularInitial" />
        <release-group name="GranularDogFood" />
        <release-group name="GranularA" />
        <release-group name="GranularB" />
      </tag>
    </tags>
  </package-ref>
</supplement>

)";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob( _))
            .Times(2)
            .WillOnce(Return(baseSuiteXML))
            .WillOnce(Return(supplementXML));
    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_TRUE(sdds3CacheUtils.checkCache({"ServerProtectionLinux-Base"},{"GranularInitial"}));

}
TEST_F(SDDS3CacheUtilsTest, testPackageConfigMissingBaseSuiteTriggersCacheClear)
{
    EXPECT_CALL(*mockFileSystem_, removeFile(_)).Times(1);
    EXPECT_CALL(*mockFileSystem_, recursivelyDeleteContentsOfDirectory(_)).Times(2);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_FALSE(sdds3CacheUtils.checkCache({"base"},{"GranularF"}));

}

TEST_F(SDDS3CacheUtilsTest, testMissingBaseSuiteTriggersCacheClear)
{
    EXPECT_CALL(*mockFileSystem_, removeFile(_)).Times(1);
    EXPECT_CALL(*mockFileSystem_, recursivelyDeleteContentsOfDirectory(_)).Times(2);
    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(1).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_FALSE(sdds3CacheUtils.checkCache({"ServerProtectionLinux-Base"},{"GranularF"}));

}

TEST_F(SDDS3CacheUtilsTest, testMissingSupplementFileTriggersCacheClear)
{

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(2).
    WillOnce(Return(true)).
    WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem_, removeFile(_)).Times(1);
    EXPECT_CALL(*mockFileSystem_, recursivelyDeleteContentsOfDirectory(_)).Times(2);
    EXPECT_CALL(*mockFileSystem_, readFile(_)).Times(1).WillRepeatedly(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base">
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="LocalRepData" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
  </package-ref>
</suite>)";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob( _))
            .Times(1)
            .WillOnce(Return(baseSuiteXML));
    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_FALSE(sdds3CacheUtils.checkCache({"ServerProtectionLinux-Base"},{"c"}));

}

TEST_F(SDDS3CacheUtilsTest, testWrongReleaseGroupTriggersCacheClear)
{
    std::string expectedBaseName{"ServerProtectionLinux-Base"};

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(EndsWith(expectedBaseName))).WillOnce(Return("base"));
    EXPECT_CALL(*mockFileSystem_, readFile(EndsWith(".dat"))).WillRepeatedly(Return("supp"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
<suite name="ServerProtectionLinux-Base">
  <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
    <platforms>
      <platform name="LINUX_ARM64" />
      <platform name="LINUX_INTEL_LIBC6" />
    </platforms>
    <supplement-ref src="LocalRepData" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
  </package-ref>
</suite>)";
    std::string supplementXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<supplement name="LocalRepData" timestamp="2023-10-25T22:55:50Z">
  <package-ref src="LocalRepData_2023.10.25.21.50.55.1.955eb5bcf6.zip" size="313172" sha256="">
    <tags>
      <tag name="LATEST_CLOUD_ENDPOINT">
        <release-group name="0" />
        <release-group name="1" />
        <release-group name="A" />
        <release-group name="B" />
        <release-group name="C" />
      </tag>
    </tags>
  </package-ref>
</supplement>
)";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob(StrEq("base")))
            .WillOnce(Return(baseSuiteXML));

    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob(StrEq("supp")))
            .Times(2)
            .WillRepeatedly(Return(supplementXML));

    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_FALSE(sdds3CacheUtils.checkCache({expectedBaseName},{"c"}));

}

TEST_F(SDDS3CacheUtilsTest, noReleaseGroupDoesNotTriggerCacheClear)
{
    std::string expectedBaseName{"ServerProtectionLinux-Base"};

    EXPECT_CALL(*mockFileSystem_, isFile(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem_, readFile(EndsWith(expectedBaseName))).WillRepeatedly(Return("base"));
    EXPECT_CALL(*mockFileSystem_, readFile(EndsWith(".dat"))).WillRepeatedly(Return("supp"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto& sdds3Wrapper = setupSdds3WrapperAndGetMock();
    std::string baseSuiteXML = R"(
    <suite name="ServerProtectionLinux-Base">
      <package-ref src="SPL-Base-Component_1.2.5.0.1506.f287b5e6e4.zip" size="43199062" sha256="">
        <platforms>
          <platform name="LINUX_ARM64" />
          <platform name="LINUX_INTEL_LIBC6" />
        </platforms>
        <supplement-ref src="LocalRepData" tag="LATEST_CLOUD_ENDPOINT" decode-path="files/" />
      </package-ref>
    </suite>)";
    std::string supplementXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
    <supplement name="LocalRepData" timestamp="2023-10-25T22:55:50Z">
      <package-ref src="LocalRepData_2023.10.25.21.50.55.1.955eb5bcf6.zip" size="313172" sha256="">
        <tags>
          <tag name="LATEST_CLOUD_ENDPOINT">
          </tag>
        </tags>
      </package-ref>
    </supplement>
    )";
    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob(StrEq("base")))
        .WillRepeatedly(Return(baseSuiteXML));

    EXPECT_CALL(sdds3Wrapper, getUnverifiedSignedBlob(StrEq("supp")))
        .WillRepeatedly(Return(supplementXML));

    SDDS3CacheUtils sdds3CacheUtils;
    EXPECT_TRUE(sdds3CacheUtils.checkCache({expectedBaseName},{"c"}));
}
