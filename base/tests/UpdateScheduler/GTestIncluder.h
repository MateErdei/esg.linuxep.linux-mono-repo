// Copyright 2018 Sophos Limited.

#pragma once

//#pragma warning(push)
//#pragma warning(disable: 4389)
//#pragma warning(disable: 6011)
//#pragma warning(disable: 28182)
#include <gtest/gtest.h>
#include <gmock/gmock.h>
//#pragma warning(pop)

// the GTest assert macros produce a "warning C6326: Potential comparison of a constant with another constant.
//#pragma warning(disable: 6326)
//#pragma warning(disable: 26818)

#include "GTestMemoryLeak.h"
#include "Macros.h"

#include <regex>

using ::testing::Action;
using ::testing::ActionInterface;
using ::testing::DefaultValue;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::InSequence;
using ::testing::InvokeArgument;
using ::testing::IsEmpty;
using ::testing::MakeAction;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgReferee;
using ::testing::SizeIs;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::TestWithParam;
using ::testing::Throw;
using ::testing::UnorderedElementsAre;
using ::testing::Values;
using ::testing::_;

//#pragma warning( push )
//#pragma warning( disable : 4100 ) // Mute unused result_listener arg.
MATCHER_P(MatchesRegex, pattern, "string to match std::regex " + ::testing::PrintToString(pattern)) {
    std::regex regex(pattern);
    return std::regex_match(arg, regex);
}

MATCHER_P(ContainsRegex, pattern, "string to contain std::regex" + ::testing::PrintToString(pattern)) {
    std::regex regex(pattern);
    return std::regex_search(arg, regex);
}
//#pragma warning( pop )

#define ASSERT_OR_EXPECT_THROW_WITH_CALLBACK(FAILURE, code, exception_type, callback)                                              \
    MULTI_LINE_MACRO_BEGIN                                                                                                         \
    auto callback_function = callback;                                                                                             \
    try                                                                                                                            \
    {                                                                                                                              \
        code;                                                                                                                      \
        FAILURE() << "Expected exception was not thrown";                                                                          \
    }                                                                                                                              \
    catch (const exception_type& e)                                                                                                \
    {                                                                                                                              \
        if (!callback_function(e))                                                                                                 \
        {                                                                                                                          \
            FAILURE() << "Failure, callback returned false";                                                                       \
        }                                                                                                                          \
    }                                                                                                                              \
    catch (...)                                                                                                                    \
    {                                                                                                                              \
        FAILURE() << "Unexpected exception type was thrown";                                                                       \
    }                                                                                                                              \
    MULTI_LINE_MACRO_END

#define ASSERT_THROW_WITH_CALLBACK(code, exception_type, callback)                                                                 \
    ASSERT_OR_EXPECT_THROW_WITH_CALLBACK(FAIL, code, exception_type, callback)

#define EXPECT_THROW_WITH_CALLBACK(code, exception_type, callback)                                                                 \
    ASSERT_OR_EXPECT_THROW_WITH_CALLBACK(ADD_FAILURE, code, exception_type, callback)