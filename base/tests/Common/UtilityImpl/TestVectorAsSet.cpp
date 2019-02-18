/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/VectorAsSet.h>
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;
using Stringvec = std::vector<std::string>;
using PairResult = std::pair<Stringvec, Stringvec>;
using ListInputOutput = std::vector<PairResult>;

TEST(TestVectorAsSet, entriesShouldSortedAndOnlyUniqueValuesAreKept) // NOLINT
{
    VectorAsSet vectorAsSet;
    ListInputOutput listInputOutput = { { { "a", "b", "c" }, { "a", "b", "c" } },
                                        { { "c", "b", "a" }, { "a", "b", "c" } },
                                        { { "c", "a", "b" }, { "a", "b", "c" } },
                                        { { "c", "c", "a", "b", "b" }, { "a", "b", "c" } } };

    for (auto inputoutput : listInputOutput)
    {
        Stringvec input = inputoutput.first;
        Stringvec expected = inputoutput.second;
        vectorAsSet.setEntries(input);
        EXPECT_EQ(vectorAsSet.entries(), expected);
    }
}

TEST(TestVectorAsSet, emptyEntriesIsEquivalentToEmptyVector) // NOLINT
{
    VectorAsSet vectorAsSet;
    EXPECT_EQ(vectorAsSet.entries(), Stringvec());
    vectorAsSet.setEntries(Stringvec());
    EXPECT_EQ(vectorAsSet.entries(), Stringvec());
}

TEST(TestVectorAsSet, entriesCanBeCopied) // NOLINT
{
    VectorAsSet vectorAsSet;
    vectorAsSet.setEntries({ "a", "b", "b" });
    VectorAsSet copyVector = vectorAsSet;
    EXPECT_EQ(copyVector.entries(), Stringvec({ "a", "b" }));
    EXPECT_EQ(vectorAsSet.entries(), Stringvec({ "a", "b" }));
    vectorAsSet.setEntries({ "a", "c" });
    EXPECT_EQ(copyVector.entries(), Stringvec({ "a", "b" }));
    EXPECT_EQ(vectorAsSet.entries(), Stringvec({ "a", "c" }));
}

TEST(TestVectorAsSet, hasEntryReturnTrueForEntriesInTheSet) // NOLINT
{
    VectorAsSet vectorAsSet;
    vectorAsSet.setEntries({ "a", "b", "b" });
    EXPECT_TRUE(vectorAsSet.hasEntry("a"));
    EXPECT_TRUE(vectorAsSet.hasEntry("b"));
    EXPECT_FALSE(vectorAsSet.hasEntry("c"));
    EXPECT_FALSE(vectorAsSet.hasEntry(""));
}

TEST(TestVectorAsSet, emptyStringCanBeAField) // NOLINT
{
    VectorAsSet vectorAsSet;
    vectorAsSet.setEntries(Stringvec({ "" }));
    EXPECT_TRUE(vectorAsSet.hasEntry(""));
}
