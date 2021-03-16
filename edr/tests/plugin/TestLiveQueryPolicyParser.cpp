/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/pluginimpl/LiveQueryPolicyParser.h>
#include <Common/FileSystem/IFileSystem.h>
#include <modules/pluginimpl/ApplicationPaths.h>

#include <Common/Helpers/LogInitializedTests.h>

#include <gtest/gtest.h>

class TestLiveQueryPolicyParser: public LogOffInitializedTests{};

bool testableGetFoldingRules(const std::string& policy, std::vector<Json::Value>& foldingRules)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getFoldingRules(attributeMap, foldingRules);
}

unsigned int testableGetDataLimit(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getDataLimit(attributeMap);
}

std::string testableGetRevId(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getRevId(attributeMap);
}

std::optional<std::string> testableGetCustomQueries(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getCustomQueries(attributeMap);
}



TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesHandlesExpectedPolicy)
{
    std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                  "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                  "   <configuration>\n"
                                  "       <scheduled>\n"
                                  "           <dailyDataLimit>100</dailyDataLimit>\n"
                                  "           <queryPacks>\n"
                                  "               <queryPack id=\"XDR\"/>\n"
                                  "               <queryPack id=\"MTR\"/>\n"
                                  "           </queryPacks>\n"
                                  "           <foldingRules>\n"
                                  "               [\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query\",\n"
                                  "                       \"values\":{\n"
                                  "                           \"column_name\": \"column_value\",\n"
                                  "                           \"column_name2\": \"column_value2\"\n"
                                  "                       }\n"
                                  "                   },\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query2\",\n"
                                  "                        \"values\":{\n"
                                  "                               \"column_name3\": \"column_value3\"\n"
                                  "                       }\n"
                                  "                   }\n"
                                  "               ]\n"
                                  "           </foldingRules>\n"
                                  "       </scheduled>\n"
                                  "   </configuration>\n"
                                  "</policy>";

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicy, foldingRules);
    EXPECT_TRUE(changeFoldingRules);
    EXPECT_EQ(foldingRules.size(), 2);
    size_t count = 0;
    for (const auto& r : foldingRules)
    {
        SCOPED_TRACE(count);

        ASSERT_TRUE(r.get("query_name", "").isString());
        ASSERT_TRUE(r.get("values", "").isObject());

        const std::string query_name = r.get("query_name", "").asString();
        const Json::Value values = r.get("values", "");

        if (count == 0)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query");
            EXPECT_STREQ(values.get("column_name", "").asString().c_str(), "column_value");
            EXPECT_STREQ(values.get("column_name2", "").asString().c_str(), "column_value2");
        }
        else if (count == 1)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query2");
            EXPECT_STREQ(values.get("column_name3", "").asString().c_str(), "column_value3");
        }

        count++;
    }
    EXPECT_EQ(count, foldingRules.size());

}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesWithNoFoldingRules)
{
    std::string liveQueryPolicyNone = "<?xml version=\"1.0\"?>\n"
                                      "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                      "   <configuration>\n"
                                      "       <scheduled>\n"
                                      "           <dailyDataLimit>100</dailyDataLimit>\n"
                                      "           <queryPacks>\n"
                                      "               <queryPack id=\"XDR\"/>\n"
                                      "               <queryPack id=\"MTR\"/>\n"
                                      "           </queryPacks>\n"
                                      "       </scheduled>\n"
                                      "   </configuration>\n"
                                      "</policy>";

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyNone, foldingRules);
    EXPECT_TRUE(changeFoldingRules);
    EXPECT_TRUE(foldingRules.empty());
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesInvalidJson)
{
    std::string liveQueryPolicyInvalid = "<?xml version=\"1.0\"?>\n"
                                         "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                         "   <configuration>\n"
                                         "       <scheduled>\n"
                                         "           <dailyDataLimit>100</dailyDataLimit>\n"
                                         "           <queryPacks>\n"
                                         "               <queryPack id=\"XDR\"/>\n"
                                         "               <queryPack id=\"MTR\"/>\n"
                                         "           </queryPacks>\n"
                                         "           <foldingRules>\n"
                                         "               blah\n"
                                         "           </foldingRules>\n"
                                         "       </scheduled>\n"
                                         "   </configuration>\n"
                                         "</policy>";

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyInvalid, foldingRules);
    EXPECT_FALSE(changeFoldingRules);
    EXPECT_TRUE(foldingRules.empty());
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesParsesAllButInvalidRules)
{
    std::string liveQueryPolicyInvalid = "<?xml version=\"1.0\"?>\n"
                                         "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                         "   <configuration>\n"
                                         "       <scheduled>\n"
                                         "           <dailyDataLimit>100</dailyDataLimit>\n"
                                         "           <queryPacks>\n"
                                         "               <queryPack id=\"XDR\"/>\n"
                                         "               <queryPack id=\"MTR\"/>\n"
                                         "           </queryPacks>\n"
                                         "           <foldingRules>\n"
                                         "               [\n"
                                         "                   {\n"
                                         "                       \"query_name\":\"test_folding_query\",\n"
                                         "                       \"not-values\":{\n"
                                         "                           \"column_name\": \"column_value\",\n"
                                         "                           \"column_name2\": \"column_value2\"\n"
                                         "                       }\n"
                                         "                   },\n"
                                         "                   {\n"
                                         "                       \"query_name\":\"test_folding_query2\",\n"
                                         "                        \"values\":{\n"
                                         "                               \"column_name3\": \"column_value3\"\n"
                                         "                       }\n"
                                         "                   }\n"
                                         "               ]\n"
                                         "           </foldingRules>\n"
                                         "       </scheduled>\n"
                                         "   </configuration>\n"
                                         "</policy>";

    std::vector<Json::Value> expected;
    Json::Value root;
    Json::Value values;
    values["column_name3"] = "column_value3";
    root["query_name"] = "test_folding_query2";
    root["values"] = values;
    expected.push_back(root);

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyInvalid, foldingRules);
    EXPECT_FALSE(changeFoldingRules);

    EXPECT_EQ(foldingRules.size(), 1);
    EXPECT_EQ(foldingRules, expected);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesGoodThenBadThenOneBadRule)
{
    // We get a policy with valid folding rules
    std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                  "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                  "   <configuration>\n"
                                  "       <scheduled>\n"
                                  "           <dailyDataLimit>100</dailyDataLimit>\n"
                                  "           <queryPacks>\n"
                                  "               <queryPack id=\"XDR\"/>\n"
                                  "               <queryPack id=\"MTR\"/>\n"
                                  "           </queryPacks>\n"
                                  "           <foldingRules>\n"
                                  "               [\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query\",\n"
                                  "                       \"values\":{\n"
                                  "                           \"column_name\": \"column_value\",\n"
                                  "                           \"column_name2\": \"column_value2\"\n"
                                  "                       }\n"
                                  "                   },\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query2\",\n"
                                  "                        \"values\":{\n"
                                  "                               \"column_name3\": \"column_value3\"\n"
                                  "                       }\n"
                                  "                   }\n"
                                  "               ]\n"
                                  "           </foldingRules>\n"
                                  "       </scheduled>\n"
                                  "   </configuration>\n"
                                  "</policy>";

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicy, foldingRules);
    EXPECT_EQ(changeFoldingRules, true);
    EXPECT_EQ(foldingRules.size(), 2);
    size_t count = 0;
    for (const auto& r : foldingRules)
    {
        SCOPED_TRACE(count);

        ASSERT_TRUE(r.get("query_name", "").isString());
        ASSERT_TRUE(r.get("values", "").isObject());

        const std::string query_name = r.get("query_name", "").asString();
        const Json::Value values = r.get("values", "");

        if (count == 0)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query");
            EXPECT_STREQ(values.get("column_name", "").asString().c_str(), "column_value");
            EXPECT_STREQ(values.get("column_name2", "").asString().c_str(), "column_value2");
        }
        else if (count == 1)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query2");
            EXPECT_STREQ(values.get("column_name3", "").asString().c_str(), "column_value3");
        }

        count++;
    }
    EXPECT_EQ(count, foldingRules.size());

    // We get another policy which has invalid rules
    std::string liveQueryPolicyInvalid = "<?xml version=\"1.0\"?>\n"
                                         "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                         "   <configuration>\n"
                                         "       <scheduled>\n"
                                         "           <dailyDataLimit>100</dailyDataLimit>\n"
                                         "           <queryPacks>\n"
                                         "               <queryPack id=\"XDR\"/>\n"
                                         "               <queryPack id=\"MTR\"/>\n"
                                         "           </queryPacks>\n"
                                         "           <foldingRules>\n"
                                         "               blah\n"
                                         "           </foldingRules>\n"
                                         "       </scheduled>\n"
                                         "   </configuration>\n"
                                         "</policy>";
    // We keep the previous folding rules
    std::vector<Json::Value> nextRules;
    bool changeFoldingRulesBadRules = testableGetFoldingRules(liveQueryPolicyInvalid, nextRules);
    EXPECT_EQ(changeFoldingRulesBadRules, false);
    EXPECT_EQ(nextRules, std::vector<Json::Value> {});

    // We get another policy with good fields but some bad rules
    std::string liveQueryPolicyInvalidRules = "<?xml version=\"1.0\"?>\n"
                                              "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                              "   <configuration>\n"
                                              "       <scheduled>\n"
                                              "           <dailyDataLimit>100</dailyDataLimit>\n"
                                              "           <queryPacks>\n"
                                              "               <queryPack id=\"XDR\"/>\n"
                                              "               <queryPack id=\"MTR\"/>\n"
                                              "           </queryPacks>\n"
                                              "           <foldingRules>\n"
                                              "               [\n"
                                              "                   {\n"
                                              "                       \"query_name\":\"test_folding_query\",\n"
                                              "                       \"not-values\":{\n"
                                              "                           \"column_name\": \"column_value\",\n"
                                              "                           \"column_name2\": \"column_value2\"\n"
                                              "                       }\n"
                                              "                   },\n"
                                              "                   {\n"
                                              "                       \"query_name\":\"test_folding_query2\",\n"
                                              "                        \"values\":{\n"
                                              "                               \"column_name3\": \"column_value3\"\n"
                                              "                       }\n"
                                              "                   }\n"
                                              "               ]\n"
                                              "           </foldingRules>\n"
                                              "       </scheduled>\n"
                                              "   </configuration>\n"
                                              "</policy>";

    std::vector<Json::Value> expected;
    Json::Value root;
    Json::Value values;
    values["column_name3"] = "column_value3";
    root["query_name"] = "test_folding_query2";
    root["values"] = values;
    expected.push_back(root);

    std::vector<Json::Value> lastRules;
    bool changeFoldingRulesSomeBadRules = testableGetFoldingRules(liveQueryPolicyInvalidRules, lastRules);
    EXPECT_EQ(changeFoldingRulesSomeBadRules, true);
    // We keep only the good rule
    EXPECT_EQ(lastRules.size(), 1);
    EXPECT_EQ(lastRules, expected);
}

TEST_F(TestLiveQueryPolicyParser, testGetDataLimit)
{ // NOLINT
    std::string liveQueryPolicy100000 = "<?xml version=\"1.0\"?>\n"
                                        "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                        "    <configuration>\n"
                                        "        <scheduled>\n"
                                        "            <dailyDataLimit>100000</dailyDataLimit>\n"
                                        "            <queryPacks>\n"
                                        "                <queryPack id=\"queryPackId\" />\n"
                                        "            </queryPacks>\n"
                                        "        </scheduled>\n"
                                        "    </configuration>\n"
                                        "</policy>";
    EXPECT_EQ(testableGetDataLimit(liveQueryPolicy100000), 100000);

    std::string liveQueryPolicy234567 = "<?xml version=\"1.0\"?>\n"
                                        "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                        "    <configuration>\n"
                                        "        <scheduled>\n"
                                        "            <dailyDataLimit>234567</dailyDataLimit>\n"
                                        "            <queryPacks>\n"
                                        "                <queryPack id=\"queryPackId\" />\n"
                                        "            </queryPacks>\n"
                                        "        </scheduled>\n"
                                        "    </configuration>\n"
                                        "</policy>";
    EXPECT_EQ(testableGetDataLimit(liveQueryPolicy234567), 234567);

    // TODO - make sure theres a unit test that processLiveQuery will throw elegantly when the xml cannot be read.
//    std::string nonsense = "asdfbhasdlfhasdflasdhfasd";
//    EXPECT_THROW(testableGetDataLimit(nonsense), std::exception);

    std::string validXmlWithMissingField = "<?xml version=\"1.0\"?>\n"
                                           "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                           "    <configuration>\n"
                                           "        <scheduled>\n"
                                           "            <notDailyDataLimit>234567</notDailyDataLimit>\n"
                                           "            <queryPacks>\n"
                                           "                <queryPack id=\"queryPackId\" />\n"
                                           "            </queryPacks>\n"
                                           "        </scheduled>\n"
                                           "    </configuration>\n"
                                           "</policy>";
    EXPECT_EQ(testableGetDataLimit(validXmlWithMissingField), 262144000);

    std::string validXmlWithInvalidFieldData = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "            <notDailyDataLimit>notAnInteger</notDailyDataLimit>\n"
                                               "            <queryPacks>\n"
                                               "                <queryPack id=\"queryPackId\" />\n"
                                               "            </queryPacks>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";
    EXPECT_EQ(testableGetDataLimit(validXmlWithInvalidFieldData), 262144000);
}

TEST_F(TestLiveQueryPolicyParser, testGetRevID)
{
    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_EQ(testableGetRevId(liveQueryPolicy100), "100");

    std::string liveQueryPolicy999999999 = "<?xml version=\"1.0\"?>\n"
                                           "<policy type=\"LiveQuery\" RevID=\"999999999\" policyType=\"56\">\n"
                                           "    <configuration>\n"
                                           "        <scheduled>\n"
                                           "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                           "            <queryPacks>\n"
                                           "                <queryPack id=\"queryPackId\" />\n"
                                           "            </queryPacks>\n"
                                           "        </scheduled>\n"
                                           "    </configuration>\n"
                                           "</policy>";

    EXPECT_EQ(testableGetRevId(liveQueryPolicy999999999), "999999999");

    std::string noRevIdPolicy = "<?xml version=\"1.0\"?>\n"
                                "<policy type=\"LiveQuery\" NotRevID=\"999999999\" policyType=\"56\">\n"
                                "    <configuration>\n"
                                "        <scheduled>\n"
                                "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                "            <queryPacks>\n"
                                "                <queryPack id=\"queryPackId\" />\n"
                                "            </queryPacks>\n"
                                "        </scheduled>\n"
                                "    </configuration>\n"
                                "</policy>";

    EXPECT_THROW(testableGetRevId(noRevIdPolicy), std::exception);

    std::string garbage = "garbage";

    EXPECT_THROW(testableGetRevId(garbage), std::exception);
}

TEST_F(TestLiveQueryPolicyParser, testUpdateCustomQueries)
{
    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "            <customQueries>\n"
                                     "                  <customQuery queryName=\"blah\">\n"
                                     "                      <description>basic query</description>\n"
                                     "                      <query>SELECT * FROM stuff</query>\n"
                                     "                      <interval>10</interval>\n"
                                     "                      <tag>DataLake</tag>\n"
                                     "                      <removed>false</removed>\n"
                                     "                      <denylist>false</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>SELECT * FROM otherstuff</query>\n"
                                     "                      <interval>5</interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    std::string expected = "{"
                           "\"schedule\":{"
                           "\"blah\":{"
                           "\"denylist\":\"false\","
                           "\"description\":\"basic query\","
                           "\"interval\":\"10\","
                           "\"query\":\"SELECT * FROM stuff\","
                           "\"removed\":\"false\","
                           "\"tag\":\"DataLake\""
                           "},"
                           "\"blah2\":{"
                           "\"denylist\":\"true\","
                           "\"description\":\"a different basic query\","
                           "\"interval\":\"5\","
                           "\"query\":\"SELECT * FROM otherstuff\","
                           "\"removed\":\"true\","
                           "\"tag\":\"stream\""
                           "}"
                           "}"
                           "}";
    EXPECT_EQ(testableGetCustomQueries(liveQueryPolicy100).value(), expected);
}

TEST_F(TestLiveQueryPolicyParser, testUpdateCustomQueriesdoesNotIncludeMalformedQueriesInJson)
{
    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "            <customQueries>\n"
                                     "                  <customQuery queryName=\"blah\">\n"
                                     "                      <description>basic query</description>\n"
                                     "                      <query>SELECT * FROM stuff</query>\n"
                                     "                      <interval>10</interval>\n"
                                     "                      <tag>DataLake</tag>\n"
                                     "                      <removed>false</removed>\n"
                                     "                      <denylist>false</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>SELECT * FROM otherstuff</query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery>\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>SELECT * FROM otherstuff</query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query></query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>  </query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    std::string expected = "{"
                           "\"schedule\":{"
                           "\"blah\":{"
                           "\"denylist\":\"false\","
                           "\"description\":\"basic query\","
                           "\"interval\":\"10\","
                           "\"query\":\"SELECT * FROM stuff\","
                           "\"removed\":\"false\","
                           "\"tag\":\"DataLake\""
                           "}"
                           "}"
                           "}";
    EXPECT_EQ(testableGetCustomQueries(liveQueryPolicy100).value(), expected);
}