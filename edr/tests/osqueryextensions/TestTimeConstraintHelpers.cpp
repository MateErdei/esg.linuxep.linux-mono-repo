/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockQueryContext.h"
#include <modules/osqueryextensions/TimeConstraintHelpers.h>
#include <gtest/gtest.h>
using namespace ::testing;

class TestTimeConstraintHelpers : public ::testing::Test
{

};

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithNoConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 0);
    ASSERT_EQ(actualResult.second, 0);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithEqualsConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> equalsSet = {"1234"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(equalsSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 1234);
    ASSERT_EQ(actualResult.second, 1234);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithMultipleEqualsConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> equalsSet = {"34567", "99999", "1234"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(equalsSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 1234);
    ASSERT_EQ(actualResult.second, 99999);

}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithLessThanConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> lessThanSet = {"13589"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(lessThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 0);
    ASSERT_EQ(actualResult.second, 13589);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithLessThanOrEqualsConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> lessThanOrEqualsSet = {"13589"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(lessThanOrEqualsSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 0);
    ASSERT_EQ(actualResult.second, 13589);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithGreaterThanConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> greaterThanSet = {"3486"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 3486);
    ASSERT_EQ(actualResult.second, 0);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithGreaterThanOrEqualsConstraint)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> greaterThanOrEqualsSet = {"3486"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(greaterThanOrEqualsSet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 3486);
    ASSERT_EQ(actualResult.second, 0);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithLessThanAndGreaterThanConstraints)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> lessThanSet = {"56482"};
    std::set<std::string> greaterThanSet = {"24647"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(lessThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    ASSERT_EQ(actualResult.first, 24647);
    ASSERT_EQ(actualResult.second, 56482);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithEqualsAndBoundaryConstraints)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> equalsSet = {"1234"};
    std::set<std::string> lessThanSet = {"24647"};
    std::set<std::string> greaterThanSet = {"56482"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(equalsSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(lessThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);
    // expect start and end times to have no boundary.
    ASSERT_EQ(actualResult.first, 0);
    ASSERT_EQ(actualResult.second, 0);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWithMultipleConstraints)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    std::set<std::string> equalsSet = {"1234", "9999"};
    std::set<std::string> lessThanSet = {"24647"};
    std::set<std::string> lessThanOrEqualSet = {"13514", "35468"};
    std::set<std::string> greaterThanSet = {"56482"};
    std::set<std::string> greaterThanOrEqualSet = {"71515"};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(equalsSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(lessThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(lessThanOrEqualSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(greaterThanOrEqualSet));

    std::pair<uint64_t, uint64_t > actualResult =  helper.GetTimeConstraints(context);

    // expect start and end times to have no boundary.
    ASSERT_EQ(actualResult.first, 0);
    ASSERT_EQ(actualResult.second, 0);
}

TEST_F(TestTimeConstraintHelpers, GetTimeConstraints_ReturnsStartAndEndTimeCorrectlyWhenConstraintIsText)
{
    OsquerySDK::TimeConstraintHelpers helper;
    StrictMock<MockQueryContext> context;

    // All context processing goes through the the same code to convert the string to an uint64.
    // Therefore setting any of the contexts will provide a valid test.
    std::set<std::string> greaterThanSet = {"HelloWorld"};
    std::set<std::string> emptySet = {};

    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::LESS_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("time", OsquerySDK::ConstraintOperator::GREATER_THAN_OR_EQUALS)).WillOnce(Return(emptySet));
    std::pair<uint64_t, uint64_t > actualResult;
    EXPECT_NO_THROW(actualResult = helper.GetTimeConstraints(context));

    ASSERT_EQ(actualResult.first, 0);
    ASSERT_EQ(actualResult.second, 0);
}