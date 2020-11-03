/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockQueryContext.h"
#include <modules/osqueryextensions/SophosServerTable.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <gtest/gtest.h>
class TestSophosServerTable : public LogOffInitializedTests{};

TEST_F(TestSophosServerTable, expectNoThrowWhenGenerateCalled)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("base/etc/sophosspl/mcs.config", "MCSID=stuff");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_NO_THROW(OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, expectEmptyValuesWhenConfigFilesDoNotExist)
{
    Tests::TempDir tempDir("/tmp");
    TableRows results;
    TableRow r;
    r["endpoint_id"] = "";
    r["customer_id"] = "";
    results.push_back(std::move(r));

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(results,OsquerySDK::SophosServerTable().Generate(context));
}
TEST_F(TestSophosServerTable, getEndpointID)
{
    Tests::TempDir tempDir("/tmp");
    TableRows results;
    TableRow r;
    r["endpoint_id"] = "stuff";
    r["customer_id"] = "";
    results.push_back(std::move(r));
    tempDir.createFile("base/etc/sophosspl/mcs.config", "MCSID=stuff");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(results,OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, getCustomerIDAndEndpointId)
{
    Tests::TempDir tempDir("/tmp");
    TableRows results;
    TableRow r;
    r["endpoint_id"] = "stuff";
    r["customer_id"] = "customer1";
    results.push_back(std::move(r));
    tempDir.createFile("base/etc/sophosspl/mcs.config", "MCSID=stuff");
    tempDir.createFile("base/etc/sophosspl/mcs_policy.config", "customerId=customer1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(results,OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, getCustomerID)
{
    Tests::TempDir tempDir("/tmp");
    TableRows results;
    TableRow r;
    r["endpoint_id"] = "";
    r["customer_id"] = "customer1";
    results.push_back(std::move(r));
    tempDir.createFile("base/etc/sophosspl/mcs_policy.config", "customerId=customer1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(results,OsquerySDK::SophosServerTable().Generate(context));
}

TEST_F(TestSophosServerTable, emptyConfigFilesHandledCorrectly)
{
    Tests::TempDir tempDir("/tmp");
    TableRows results;
    TableRow r;
    r["endpoint_id"] = "";
    r["customer_id"] = "";
    results.push_back(std::move(r));
    tempDir.createFile("base/etc/sophosspl/mcs.config", "");
    tempDir.createFile("base/etc/sophosspl/mcs_policy.config", "");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    MockQueryContext context;
    EXPECT_EQ(results,OsquerySDK::SophosServerTable().Generate(context));
}