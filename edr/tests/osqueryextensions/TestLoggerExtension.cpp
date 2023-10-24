// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "osqueryextensions/LoggerExtension.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include <gtest/gtest.h>

class TestLoggerExtension : public LogOffInitializedTests
{
public:
    TestLoggerExtension(): m_loggerExtension("","","","","","",10,10, [](){},10, 10)
    {
    };

    LoggerExtension m_loggerExtension;
};

TEST_F(TestLoggerExtension, compareFoldingRulesReturnsFalseWhenComparingEmptyRuleSets) // NOLINT
{
    std::vector<Json::Value> emptyRules;
    EXPECT_FALSE(m_loggerExtension.compareFoldingRules(emptyRules));
}

TEST_F(TestLoggerExtension, compareFoldingRulesReturnsTrueWhenComparingEmptyAgainstPopulatedRuleSet) // NOLINT
{
    std::vector<Json::Value> rules;
    Json::Value root;
    Json::Value values;
    values["column_name3"] = "column_value3";
    root["query_name"] = "test_folding_query2";
    root["values"] = values;
    rules.push_back(root);
    EXPECT_TRUE(m_loggerExtension.compareFoldingRules(rules));
}

TEST_F(TestLoggerExtension, compareFoldingRulesReturnsFalseWhenComparingMatchingPopulatedRuleSets) // NOLINT
{
    std::vector<Json::Value> rules;
    Json::Value root;
    Json::Value values;
    values["column_name3"] = "column_value3";
    root["query_name"] = "test_folding_query2";
    root["values"] = values;
    rules.push_back(root);
    m_loggerExtension.setFoldingRules(rules);
    EXPECT_FALSE(m_loggerExtension.compareFoldingRules(rules));
}

TEST_F(TestLoggerExtension, compareFoldingRulesReturnsTrueWhenComparingDifferentPopulatedRuleSets) // NOLINT
{
    std::vector<Json::Value> rules1;
    Json::Value root1;
    Json::Value values1;
    values1["column_name1"] = "column_value1";
    root1["query_name"] = "query1";
    root1["values"] = values1;
    rules1.push_back(root1);
    m_loggerExtension.setFoldingRules(rules1);

    std::vector<Json::Value> rules2;
    Json::Value root2;
    Json::Value values2;
    values2["column_name2"] = "column_value2";
    root2["query_name"] = "query2";
    root2["values"] = values2;
    rules2.push_back(root2);

    EXPECT_TRUE(m_loggerExtension.compareFoldingRules(rules2));
}