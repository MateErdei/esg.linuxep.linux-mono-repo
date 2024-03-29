// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileSystem.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/MockApiBaseServices.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#include "base/tests/Common/Helpers/TempDir.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockApiBaseServices.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/TempDir.h"
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include <gtest/gtest.h>
#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/Telemetry/Telemetry.h"

class TestablePluginAdapter : public Plugin::PluginAdapter
{
public:
    explicit TestablePluginAdapter(std::shared_ptr<Plugin::QueueTask> queueTask) :
            Plugin::PluginAdapter(
                    queueTask,
                    std::unique_ptr<Common::PluginApi::IBaseServiceApi>(new MockApiBaseServices()),
                    std::make_shared<Plugin::PluginCallback>(queueTask))
    {
    }

    std::tuple<std::string, std::string> readXml(const std::string& xml)
    {
        return Plugin::PluginAdapter::readXml(xml);
    }

    std::string createJsonFile(const std::string& xml, const std::string& id)
    {
        return Plugin::PluginAdapter::createJsonFile(xml, id);
    }

    void setMcsConfigFilepath(std::string tempDir)
    {
        m_mcsConfigPath = tempDir + "/mcs.config";
    }
};

Plugin::Task defaultResponseTask()
{
    Plugin::Task task;
    task.m_taskType = Plugin::Task::TaskType::Response;
    task.m_content = "a";
    task.m_correlationId = "id";
    return task;
}

class TestPluginAdapter : public LogOffInitializedTests
{
protected:
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }
};

TEST_F(TestPluginAdapter, readXmlReturnsEmptyUrlAndThumbprintWhenNotPresent)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    auto [url, thumbprint] = pluginAdapter.readXml("asd");
    EXPECT_EQ(url, "");
    EXPECT_EQ(thumbprint, "");
}

TEST_F(TestPluginAdapter, readXmlReturnsUrlAndThumbprintCorrectly)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";
    auto [url, thumbprint] = pluginAdapter.readXml(content);
    EXPECT_EQ(url, "dummyurl");
    EXPECT_EQ(thumbprint, "examplethumbprint");
}

TEST_F(TestPluginAdapter, readXmlReturnsUrlButNotThumbprintWhenThumbprintIsEmpty)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint></thumbprint>
        </action>)";
    auto [url, thumbprint] = pluginAdapter.readXml(content);
    EXPECT_EQ(url, "dummyurl");
    EXPECT_EQ(thumbprint, "");
}

TEST_F(TestPluginAdapter, readXmlReturnsUrlButNotThumbprintWhenOnlyUrlIsPresent)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        </action>)";
    auto [url, thumbprint] = pluginAdapter.readXml(content);
    EXPECT_EQ(url, "dummyurl");
    EXPECT_EQ(thumbprint, "");
}

TEST_F(TestPluginAdapter, readXmlReturnsThumbprintButNotUrlWhenUrlIsEmpty)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url></url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";
    auto [url, thumbprint] = pluginAdapter.readXml(content);
    EXPECT_EQ(url, "");
    EXPECT_EQ(thumbprint, "examplethumbprint");
}

TEST_F(TestPluginAdapter, readXmlReturnsThumbprintButNotUrlWhenOnlyThumbprintIsPresent)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";
    auto [url, thumbprint] = pluginAdapter.readXml(content);
    EXPECT_EQ(url, "");
    EXPECT_EQ(thumbprint, "examplethumbprint");
}

TEST_F(TestPluginAdapter, createJsonFileConvertsXmlToJsonAndCreatesFile)
{
    Plugin::Telemetry::initialiseTelemetry();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getVersionIniFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/etc/os-release")).WillOnce(Return(false));
    std::vector<std::string> list = {"jwt_token=exampletoken","device_id=exampledeviceid","tenant_id=exampletenantid"};
    EXPECT_CALL(*filesystemMock, readLines(_)).WillRepeatedly(Return(list));
    std::string jsonContent =R"Sophos({"device_id":"exampledeviceid","jwt_token":"exampletoken","tenant_id":"exampletenantid","thumbprint":"examplethumbprint","url":"dummyurl","user_agent":"live-term/BAD-VERSION (Linux)"})Sophos";
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_, jsonContent, _)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillRepeatedly(Return(false));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";
    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_FALSE(filename.empty());
}

TEST_F(TestPluginAdapter, createJsonFileReturnsEmptyIfEitherUrlOrThumbprintIsMissingFromActionXml)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        </action>)";
    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());

    content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";
    filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());
}

TEST_F(TestPluginAdapter, createJsonFileReturnsEmptyIfjwt_tokenIsMissingFromMcsConfig)
{
    Plugin::Telemetry::initialiseTelemetry();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    std::vector<std::string> list = {"device_id=exampledeviceid", "tenant_id=exampletenantid"};
    EXPECT_CALL(*filesystemMock, readLines(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillRepeatedly(
            Return(false));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string content = R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";

    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());

    filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());
}

TEST_F(TestPluginAdapter, createJsonFileReturnsEmptyIftenant_idIsMissingFromMcsConfig)
{
    Plugin::Telemetry::initialiseTelemetry();

    auto filesystemMock = new StrictMock<MockFileSystem>();

    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    std::vector<std::string> list = {"device_id=exampledeviceid", "jwt_token=exampletenantid"};
    EXPECT_CALL(*filesystemMock, readLines(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillRepeatedly(
            Return(false));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string content = R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";

    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());

    filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());
}

TEST_F(TestPluginAdapter, createJsonFileReturnsEmptyIfdevice_idIsMissingFromMcsConfig)
{
    Plugin::Telemetry::initialiseTelemetry();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    std::vector<std::string> list = {"jwt_token=exampledeviceid", "tenant_id=exampletenantid"};
    EXPECT_CALL(*filesystemMock, readLines(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillRepeatedly(
            Return(false));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string content = R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";

    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());

    filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());
}

TEST_F(TestPluginAdapter, createJsonFileReturnsEmptyIfMcsConfigIsEmpty)
{
    Plugin::Telemetry::initialiseTelemetry();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string jsonContent = R"({"device_id":"exampledeviceid","jwt_token":"exampletoken","tenant_id":"exampletenantid","thumbprint":"examplethumbprint","url":"dummyurl"})";
    EXPECT_CALL(*filesystemMock, writeFileAtomically(_, jsonContent, _)).Times(0);
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    std::vector<std::string> list = {};
    EXPECT_CALL(*filesystemMock, readLines(_)).WillRepeatedly(Return(list));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillRepeatedly(
            Return(false));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string content = R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";

    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());

    filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());
}

TEST_F(TestPluginAdapter, createJsonFileReturnsEmptyStringWhenItFailsToCreateFile)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string jsonContent =R"({"thumbprint":"examplethumbprint","url":"dummyurl"})";
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(false));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::string content =R"(<action type="sophos.mgt.action.InitiateLiveTerminal">
        <url>dummyurl</url>
        <thumbprint>examplethumbprint</thumbprint>
        </action>)";
    std::string filename = pluginAdapter.createJsonFile(content, "id");
    EXPECT_TRUE(filename.empty());
}
