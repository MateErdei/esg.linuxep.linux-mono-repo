// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include <modules/pluginimpl/LiveQueryPolicyParser.h>
#include <Common/FileSystem/IFileSystem.h>
#include <modules/pluginimpl/ApplicationPaths.h>

#include <Common/Helpers/LogInitializedTests.h>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

class TestLiveQueryPolicyParser: public LogInitializedTests{};

bool testableGetFoldingRules(const std::string& policy, std::vector<Json::Value>& foldingRules)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getFoldingRules(attributeMap, foldingRules);
}

unsigned long long int testableGetDataLimit(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getDataLimit(attributeMap);
}

std::string testableGetRevId(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getRevId(attributeMap);
}

bool testableGetCustomQueries(const std::string& policy, std::optional<std::string>& customQueries)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getCustomQueries(attributeMap,customQueries);
}

bool testableGetScheduledQueriesEnabledInPolicy(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getScheduledQueriesEnabledInPolicy(attributeMap);
}

std::vector<std::string> testableGetEnabledQueryPacksInPolicy(const std::string& policy)
{
    Common::XmlUtilities::AttributesMap attributeMap = Common::XmlUtilities::parseXml(policy);
    return Plugin::getEnabledQueryPacksInPolicy(attributeMap);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesWithSwappedTypesInJson)
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
                                  "                       \"query_name\":1234,\n"
                                  "                       \"values\":{\n"
                                  "                           \"column_name\": \"column_value\",\n"
                                  "                           \"column_name2\": \"column_value2\"\n"
                                  "                       }\n"
                                  "                   },\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query2\",\n"
                                  "                        \"values\":{\n"
                                  "                               \"column_name3\": 12345\n"
                                  "                       }\n"
                                  "                   },\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query3\",\n"
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
    EXPECT_EQ(changeFoldingRules, false);
    EXPECT_EQ(foldingRules.size(), 0U);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesForTheSameQuery)
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
                                  "                           \"column_name\": \"column_value\"\n"
                                  "                       }\n"
                                  "                   },\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query\",\n"
                                  "                        \"values\":{\n"
                                  "                               \"column_name\": \"column_value\"\n"
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
    EXPECT_EQ(foldingRules.size(), 2U);
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
        }
        else if (count == 1)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query");
            EXPECT_STREQ(values.get("column_name", "").asString().c_str(), "column_value");
        }

        count++;
    }
    EXPECT_EQ(count, foldingRules.size());

}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesWithNoQueryNames)
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
                                         "                        \"values\":{\n"
                                         "                               \"column_name3\": \"column_value3\"\n"
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
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyInvalid, foldingRules);
    EXPECT_EQ(changeFoldingRules, true);
    EXPECT_EQ(foldingRules.size(),1U);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesWithNoValues)
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
                                         "                       \"query_name\":\"test_folding_query\"\n"
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
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyInvalid, foldingRules);
    EXPECT_EQ(changeFoldingRules, true);
    EXPECT_EQ(foldingRules.size(),1U);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesEmptyJson)
{
    std::string liveQueryPolicyEmptyJson = "<?xml version=\"1.0\"?>\n"
                                           "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                           "   <configuration>\n"
                                           "       <scheduled>\n"
                                           "           <dailyDataLimit>100</dailyDataLimit>\n"
                                           "           <queryPacks>\n"
                                           "               <queryPack id=\"XDR\"/>\n"
                                           "               <queryPack id=\"MTR\"/>\n"
                                           "           </queryPacks>\n"
                                           "           <foldingRules>\n"
                                           "               []\n"
                                           "           </foldingRules>\n"
                                           "       </scheduled>\n"
                                           "   </configuration>\n"
                                           "</policy>";

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyEmptyJson, foldingRules);
    EXPECT_EQ(changeFoldingRules, true);
    EXPECT_TRUE(foldingRules.empty());
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesWithVeryLargeJson)
{
    std::string liveQueryPolicyWithLargeJson = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                               "   <configuration>\n"
                                               "       <scheduled>\n"
                                               "           <dailyDataLimit>100</dailyDataLimit>\n"
                                               "           <queryPacks>\n"
                                               "               <queryPack id=\"XDR\"/>\n"
                                               "               <queryPack id=\"MTR\"/>\n"
                                               "           </queryPacks>\n"
                                               "           <foldingRules>\n"
                                               "               [\n";
    // Adding 5.000 different folding rules
    for(int i = 1; i <= 5000; i++)
    {
        std::string foldingRule =     "                   {\n"
                                      "                       \"query_name\":\"test_folding_query" + std::to_string(i) + "\",\n"
                                      "                       \"values\":{\n"
                                      "                           \"column_name" + std::to_string(i+5001) + "\": \"column_value" + std::to_string(i+5001) + "\",\n"
                                      "                           \"column_name2" + std::to_string(i+5001) +  "\": \"column_value2" +  std::to_string(i+5001) + "\"\n"
                                      "                       }\n"
                                      "                   },\n";

        liveQueryPolicyWithLargeJson = liveQueryPolicyWithLargeJson + foldingRule;
    }

    // Adding one more folding rule and the rest of the policy
    std::string restOfPolicy =                 "                   {\n"
                                               "                       \"query_name\":\"test_folding_query5002\",\n"
                                               "                        \"values\":{\n"
                                               "                               \"column_name10003\": \"column_value10003\"\n"
                                               "                       }\n"
                                               "                   }\n"
                                               "               ]\n"
                                               "           </foldingRules>\n"
                                               "       </scheduled>\n"
                                               "   </configuration>\n"
                                               "</policy>";

    liveQueryPolicyWithLargeJson = liveQueryPolicyWithLargeJson + restOfPolicy;

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyWithLargeJson, foldingRules);
    EXPECT_EQ(changeFoldingRules, true);
    EXPECT_EQ(foldingRules.size(), 5001U);

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
    EXPECT_EQ(foldingRules.size(), 2U);
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
    EXPECT_TRUE(changeFoldingRules);

    EXPECT_EQ(foldingRules.size(), 1U);
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
    EXPECT_EQ(foldingRules.size(), 2U);
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
    EXPECT_EQ(lastRules.size(), 1U);
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
    EXPECT_EQ(testableGetDataLimit(liveQueryPolicy100000), 100000U);

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
    EXPECT_EQ(testableGetDataLimit(liveQueryPolicy234567), 234567U);
    std::string liveQueryPolicy1000GB = "<?xml version=\"1.0\"?>\n"
                                        "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                        "    <configuration>\n"
                                        "        <scheduled>\n"
                                        "            <dailyDataLimit>100000000000</dailyDataLimit>\n"
                                        "            <queryPacks>\n"
                                        "                <queryPack id=\"queryPackId\" />\n"
                                        "            </queryPacks>\n"
                                        "        </scheduled>\n"
                                        "    </configuration>\n"
                                        "</policy>";
    EXPECT_EQ(testableGetDataLimit(liveQueryPolicy1000GB), 100000000000ULL);
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
    EXPECT_EQ(testableGetDataLimit(validXmlWithMissingField), 262144000U);

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
    EXPECT_EQ(testableGetDataLimit(validXmlWithInvalidFieldData), 262144000U);
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

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryCustomQueriesTypesMismatched)
{
    std::string liveQueryPolicyMismatched =  "<?xml version=\"1.0\"?>\n"
                                             "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                             "    <configuration>\n"
                                             "        <scheduled>\n"
                                             "            <dailyDataLimit>100000</dailyDataLimit>\n"
                                             "            <queryPacks>\n"
                                             "                <queryPack id=\"queryPackId\" />\n"
                                             "            </queryPacks>\n"
                                             "            <customQueries>\n"
                                             "               blah\n"
                                             "            </customQueries>\n"
                                             "        </scheduled>\n"
                                             "    </configuration>\n"
                                             "</policy>";


    std::optional<std::string> customQueries;
    bool customQueriesChanged = testableGetCustomQueries(liveQueryPolicyMismatched, customQueries);
    EXPECT_EQ(customQueriesChanged, false);
    EXPECT_FALSE(customQueries.has_value());
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryCustomQueriesAbsurdlyLargeNumber)
{
    std::string liveQueryPolicyAbsurd = "<?xml version=\"1.0\"?>\n"
                                        "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                        "    <configuration>\n"
                                        "        <scheduled>\n"
                                        "            <dailyDataLimit>100000</dailyDataLimit>\n"
                                        "            <queryPacks>\n"
                                        "                <queryPack id=\"queryPackId\" />\n"
                                        "            </queryPacks>\n"
                                        "            <customQueries>\n";

    // adding 500 different custom queries
    for (int i=1; i <= 500; i++)
    {
        std::string customQuery =    "                  <customQuery queryName=\"blah" + std::to_string(i) + "\">\n"
                                     "                      <description>basic query</description>\n"
                                     "                      <query>SELECT * FROM stuff</query>\n"
                                     "                      <interval>10</interval>\n"
                                     "                      <tag>DataLake</tag>\n"
                                     "                      <removed>false</removed>\n"
                                     "                      <denylist>false</denylist>\n"
                                     "                  </customQuery>\n";

        liveQueryPolicyAbsurd = liveQueryPolicyAbsurd + customQuery;
    }

    std::string restOfPolicy =       "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    liveQueryPolicyAbsurd = liveQueryPolicyAbsurd + restOfPolicy;

    std::string expected =  "{"
                            "\"schedule\":{";

    // creating expectations of 500 custom queries
    for (int i=1; i <= 499; i++)
    {
        std::string customQueryExpected =    "\"blah" + std::to_string(i) + "\":{"
                                                                            "\"denylist\":\"false\","
                                                                            "\"description\":\"basic query\","
                                                                            "\"interval\":\"10\","
                                                                            "\"query\":\"SELECT * FROM stuff\","
                                                                            "\"removed\":\"false\","
                                                                            "\"tag\":\"DataLake\""
                                                                            "},";

        expected = expected + customQueryExpected;
    }

    // adding the ending of the expectation
    std::string expectedEnding = "\"blah500\":{"
                                 "\"denylist\":\"false\","
                                 "\"description\":\"basic query\","
                                 "\"interval\":\"10\","
                                 "\"query\":\"SELECT * FROM stuff\","
                                 "\"removed\":\"false\","
                                 "\"tag\":\"DataLake\""
                                 "}"
                                 "}"
                                 "}";

    expected = expected + expectedEnding;

    std::optional<std::string> result;
    bool customQueriesChanged = testableGetCustomQueries(liveQueryPolicyAbsurd, result);
    EXPECT_EQ(customQueriesChanged, true);
    EXPECT_TRUE(result.has_value());

    // comparing the objects to see if all expected queries are present
    nlohmann::json j = nlohmann::json::parse(result.value());
    nlohmann::json g = nlohmann::json::parse(expected);
    EXPECT_EQ(j, g);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryCustomQueriesInvalidInterval)
{
    std::string liveQueryPolicyInvalidInterval = "<?xml version=\"1.0\"?>\n"
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
                                                 "                      <interval>-10</interval>\n"
                                                 "                      <tag>DataLake</tag>\n"
                                                 "                      <removed>false</removed>\n"
                                                 "                      <denylist>false</denylist>\n"
                                                 "                  </customQuery>\n"
                                                 "                  <customQuery queryName=\"blah2\">\n"
                                                 "                      <description>a different basic query</description>\n"
                                                 "                      <query>SELECT * FROM otherstuff</query>\n"
                                                 "                      <interval>555555555555555555</interval>\n"
                                                 "                      <tag>stream</tag>\n"
                                                 "                      <removed>true</removed>\n"
                                                 "                      <denylist>true</denylist>\n"
                                                 "                  </customQuery>\n"
                                                 "                  <customQuery queryName=\"blah3\">\n"
                                                 "                      <description>a different basic query</description>\n"
                                                 "                      <query>SELECT * FROM otherstuff</query>\n"
                                                 "                      <interval>10.55555</interval>\n"
                                                 "                      <tag>stream</tag>\n"
                                                 "                      <removed>true</removed>\n"
                                                 "                      <denylist>true</denylist>\n"
                                                 "                  </customQuery>\n"
                                                 "                  <customQuery queryName=\"blah4\">\n"
                                                 "                      <description>a different basic query</description>\n"
                                                 "                      <query>SELECT * FROM otherstuff</query>\n"
                                                 "                      <interval>10/3</interval>\n"
                                                 "                      <tag>stream</tag>\n"
                                                 "                      <removed>true</removed>\n"
                                                 "                      <denylist>true</denylist>\n"
                                                 "                  </customQuery>\n"
                                                 "            </customQueries>\n"
                                                 "        </scheduled>\n"
                                                 "    </configuration>\n"
                                                 "</policy>";

    std::string expected =      "{"
                                "\"schedule\":{"
                                "\"blah\":{"
                                "\"denylist\":\"false\","
                                "\"description\":\"basic query\","
                                "\"interval\":\"-10\","
                                "\"query\":\"SELECT * FROM stuff\","
                                "\"removed\":\"false\","
                                "\"tag\":\"DataLake\""
                                "},"
                                "\"blah2\":{"
                                "\"denylist\":\"true\","
                                "\"description\":\"a different basic query\","
                                "\"interval\":\"555555555555555555\","
                                "\"query\":\"SELECT * FROM otherstuff\","
                                "\"removed\":\"true\","
                                "\"tag\":\"stream\""
                                "},"
                                "\"blah3\":{"
                                "\"denylist\":\"true\","
                                "\"description\":\"a different basic query\","
                                "\"interval\":\"10.55555\","
                                "\"query\":\"SELECT * FROM otherstuff\","
                                "\"removed\":\"true\","
                                "\"tag\":\"stream\""
                                "},"
                                "\"blah4\":{"
                                "\"denylist\":\"true\","
                                "\"description\":\"a different basic query\","
                                "\"interval\":\"10/3\","
                                "\"query\":\"SELECT * FROM otherstuff\","
                                "\"removed\":\"true\","
                                "\"tag\":\"stream\""
                                "}"
                                "}"
                                "}";

    std::optional<std::string> customQueries;
    bool customQueriesChanged = testableGetCustomQueries(liveQueryPolicyInvalidInterval, customQueries);
    EXPECT_EQ(customQueriesChanged, true);
    EXPECT_EQ(customQueries.value(), expected);
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

    std::optional<std::string> customQueries;
    bool customQueriesChanged = testableGetCustomQueries(liveQueryPolicy100, customQueries);
    EXPECT_EQ(customQueriesChanged, true);
    EXPECT_EQ(customQueries.value(), expected);
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

    std::optional<std::string> customQueries;
    bool customQueriesChanged = testableGetCustomQueries(liveQueryPolicy100, customQueries);
    EXPECT_EQ(customQueriesChanged, true);
    EXPECT_EQ(customQueries.value(), expected);
}

TEST_F(TestLiveQueryPolicyParser, testGetScheduledQueriesEnabledInPolicy)
{
    std::string liveQueryPolicyWithEnabled =    "<?xml version=\"1.0\"?>\n"
                                                "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                                "    <configuration>\n"
                                                "        <scheduled>\n"
                                                "            <enabled>true</enabled>\n"
                                                "        </scheduled>\n"
                                                "    </configuration>\n"
                                                "</policy>";

    std::string liveQueryPolicyWithDisabled =   "<?xml version=\"1.0\"?>\n"
                                                "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                                "    <configuration>\n"
                                                "        <scheduled>\n"
                                                "            <enabled>false</enabled>\n"
                                                "        </scheduled>\n"
                                                "    </configuration>\n"
                                                "</policy>";

    std::string liveQueryPolicyWithNotPresent = "<?xml version=\"1.0\"?>\n"
                                                "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                                "    <configuration>\n"
                                                "        <scheduled>\n"
                                                "        </scheduled>\n"
                                                "    </configuration>\n"
                                                "</policy>";

    std::string liveQueryPolicyWithWrongType = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "            <enabled>98176496132</enabled>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";


    EXPECT_TRUE(testableGetScheduledQueriesEnabledInPolicy(liveQueryPolicyWithEnabled));
    EXPECT_FALSE(testableGetScheduledQueriesEnabledInPolicy(liveQueryPolicyWithDisabled));
    EXPECT_FALSE(testableGetScheduledQueriesEnabledInPolicy(liveQueryPolicyWithNotPresent));
    EXPECT_FALSE(testableGetScheduledQueriesEnabledInPolicy(liveQueryPolicyWithWrongType));
}

TEST_F(TestLiveQueryPolicyParser, testGetEnabledQueryPacksInPolicy) {
    std::string liveQueryPolicyWithMtr = "<?xml version=\"1.0\"?>\n"
                                         "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                         "    <configuration>\n"
                                         "        <scheduled>\n"
                                         "           <queryPacks>\n"
                                         "               <queryPack id=\"MTR\" />\n"
                                         "           </queryPacks>\n"
                                         "        </scheduled>\n"
                                         "    </configuration>\n"
                                         "</policy>";

    EXPECT_EQ(testableGetEnabledQueryPacksInPolicy(liveQueryPolicyWithMtr), std::vector<std::string>{"MTR"});

    std::string liveQueryPolicyWithMtrAndXDR = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "           <queryPacks>\n"
                                               "               <queryPack id=\"MTR\" />\n"
                                               "               <queryPack id=\"XDR\" />\n"
                                               "           </queryPacks>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";
    EXPECT_EQ(testableGetEnabledQueryPacksInPolicy(liveQueryPolicyWithMtrAndXDR), (std::vector<std::string>{"MTR", "XDR"}));

    std::string liveQueryPolicyWithMtrAndXDRAndOther = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "           <queryPacks>\n"
                                               "               <queryPack id=\"MTR\" />\n"
                                               "               <queryPack id=\"XDR\" />\n"
                                               "               <queryPack id=\"OTHER\" />\n"
                                               "           </queryPacks>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";
    EXPECT_EQ(testableGetEnabledQueryPacksInPolicy(liveQueryPolicyWithMtrAndXDR), (std::vector<std::string>{"MTR", "XDR"}));

    std::string liveQueryPolicyWithWrongType = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "           <queryPacks>\n"
                                               "               <queryPack id=\"1\" />\n"
                                               "               <queryPack id=\"2\" />\n"
                                               "               <queryPack id=\"MTR\" />\n"
                                               "           </queryPacks>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";
    EXPECT_EQ(testableGetEnabledQueryPacksInPolicy(liveQueryPolicyWithWrongType), (std::vector<std::string>{"MTR"}));

    std::string liveQueryPolicyWithNoPacksSection = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";
    EXPECT_EQ(testableGetEnabledQueryPacksInPolicy(liveQueryPolicyWithNoPacksSection), (std::vector<std::string>{}));
}

TEST_F(TestLiveQueryPolicyParser, testUpdateCustomQueriesGoodBadGoodScenario)
{
    // get good policy with valid custom queries
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

    std::optional<std::string> customQueries;
    bool customQueriesChanged = testableGetCustomQueries(liveQueryPolicy100, customQueries);
    EXPECT_EQ(customQueriesChanged, true);
    EXPECT_TRUE(customQueries.has_value());
    EXPECT_EQ(customQueries.value(), expected);

    // get policy with bad custom query field
    std::string liveQueryPolicyInvalidCustomQueries = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "            <customQueries>\n"
                                     "                  blah\n"
                                     "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    std::optional<std::string> customQueriesNext;
    bool customQueriesChanged2 = testableGetCustomQueries(liveQueryPolicyInvalidCustomQueries, customQueriesNext);
    EXPECT_EQ(customQueriesChanged2, false);
    EXPECT_FALSE(customQueriesNext.has_value());

    // get policy with some good custom queries
    std::string liveQueryPolicySomeGoodQueries = "<?xml version=\"1.0\"?>\n"
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

    std::string expected2 = "{"
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

    std::optional<std::string> customQueriesLast;
    bool customQueriesChanged3 = testableGetCustomQueries(liveQueryPolicySomeGoodQueries, customQueriesLast);
    EXPECT_EQ(customQueriesChanged3, true);
    EXPECT_TRUE(customQueriesLast.has_value());
    EXPECT_EQ(customQueriesLast.value(), expected2);
}

TEST_F(TestLiveQueryPolicyParser, testProcessLiveQueryFoldingRulesWithRegex)
{

    std::string liveQueryPolicyWithRegex{R"(<?xml version="1.0"?>
                                 <policy type="LiveQuery" RevID="abc123" policyType="56">
                                    <configuration>
                                        <scheduled>
                                            <dailyDataLimit>100</dailyDataLimit>
                                            <queryPacks>
                                                <queryPack id="XDR"/>
                                                <queryPack id="MTR"/>
                                            </queryPacks>
                                            <foldingRules>
                                                [
                                                    {
                                                        "query_name":"test_folding_query",
                                                         "regex":{
                                                                "column_name": "[0-9]+"
                                                        }
                                                    }
                                                ]
                                            </foldingRules>
                                        </scheduled>
                                    </configuration>
                                 </policy>)"};

    std::vector<Json::Value> foldingRules;
    bool changeFoldingRules = testableGetFoldingRules(liveQueryPolicyWithRegex, foldingRules);
    EXPECT_EQ(changeFoldingRules, true);
    EXPECT_EQ(foldingRules.size(),1U);
    EXPECT_EQ(foldingRules[0]["query_name"], "test_folding_query");
    EXPECT_EQ(foldingRules[0]["regex"]["column_name"], "[0-9]+");
}