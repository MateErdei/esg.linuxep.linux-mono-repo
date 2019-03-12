/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/EventTypes/SophosString.h>
#include <gmock/gmock.h>

using namespace Common::EventTypes;


class TestSophosString : public ::testing::Test
{
public:
    TestSophosString() = default;

    std::string generateTestString(const SophosString& str)
    {
        return str.str();
    }

    SophosString generateTestSophosString(const std::string& str)
    {
        return str;
    }

    uint64_t m_maxKilobytes = 5*1024;
};


TEST_F( // NOLINT
        TestSophosString,
        testSophosStringCopyConstuctorUsingCharStoresValueCorrectly)
{
    SophosString testString("asd");
    ASSERT_EQ(testString.str(), "asd");
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringCopyConstuctorUsingStringStoresValueCorrectly)
{
    std::string basicString("basic");
    SophosString testString(basicString);
    ASSERT_EQ(testString.str(), basicString);
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringCopyConstuctorUsingSophosStringStoresValueCorrectly)
{
    SophosString originalSophosString("basic");
    SophosString testString(originalSophosString);
    ASSERT_EQ(testString.str(), originalSophosString.str());
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringEqualsOperatorEvaluatesTheSameStringCorrectly)
{
    SophosString testString1("equal");
    SophosString testString2("equal");

    ASSERT_EQ(testString1, testString2);

}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringNotEqualsOperatorEvaluatesDifferentStringsCorrectly)
{
    SophosString testString1("equal");
    SophosString testString2("notthesame");

    ASSERT_NE(testString1, testString2);
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringAssignOperatorAssignsFromStringCorrectly)
{
    std::string stdString("string");
    SophosString newString = stdString;
    ASSERT_EQ(newString.str(), stdString);
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringAssignOperatorAssignsFromSophosStringCorrectly)
{
    SophosString testString("string");
    SophosString newString = testString;
    ASSERT_EQ(newString, testString);
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringAssignOperatorAssignsFromCharStringCorrectly)
{
    char  charArray[] = "array";
    SophosString newString = charArray;
    ASSERT_EQ(newString, charArray);
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringStreamOperatorStreamsStringCorrectly)
{
    SophosString newString = "stream";
    std::stringstream streamedString;
    streamedString << newString;
    ASSERT_EQ(newString, streamedString.str());
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringGraterThanMaxKilobytesBy1IsTruncatedCorrectly)
{
    std::stringstream expectedString;

    for(uint64_t i = 0; i < m_maxKilobytes; i++ )
    {
        expectedString << "a";
    }

    std::stringstream actualString;

    for(uint64_t i = 0; i < m_maxKilobytes + 1; i++ )
    {
        actualString << "a";
    }

    SophosString actualSophosString(expectedString.str());


    ASSERT_EQ(expectedString.str(), actualSophosString.str());
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringGraterThanMaxKilobytesBy1000IsTruncatedCorrectly)
{
    std::stringstream expectedString;

    for(uint64_t i = 0; i < m_maxKilobytes; i++ )
    {
        expectedString << "a";
    }

    std::stringstream actualString;

    for(uint64_t i = 0; i < m_maxKilobytes + 1000; i++ )
    {
        actualString << "a";
    }

    SophosString actualSophosString(expectedString.str());


    ASSERT_EQ(expectedString.str(), actualSophosString.str());
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringEqualsMaxKilobytesIsNotTruncated)
{
    std::stringstream expectedString;

    for(uint64_t i = 0; i < m_maxKilobytes; i++ )
    {
        expectedString << "a";
    }

    std::stringstream actualString;

    for(uint64_t i = 0; i < m_maxKilobytes; i++ )
    {
        actualString << "a";
    }

    SophosString actualSophosString(expectedString.str());


    ASSERT_EQ(expectedString.str(), actualSophosString.str());
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringLessThanMaxKilobytesIsNotTruncated)
{
    std::stringstream expectedString;

    for(uint64_t i = 0; i < 100; i++ )
    {
        expectedString << "a";
    }

    std::stringstream actualString;

    for(uint64_t i = 0; i < 100; i++ )
    {
        actualString << "a";
    }

    SophosString actualSophosString(expectedString.str());


    ASSERT_EQ(expectedString.str(), actualSophosString.str());
}

TEST_F( // NOLINT
        TestSophosString,
        testSophosStringCanBePassedToFromMethodCorrectly)
{
    SophosString testSophosString1 = generateTestSophosString("TestString1");

    std::string testString1 = generateTestString(testSophosString1);

    std::string testString2 = generateTestSophosString("TestString2");

    ASSERT_EQ(testString1, testSophosString1.str());
    ASSERT_EQ("TestString2", testString2);
}


