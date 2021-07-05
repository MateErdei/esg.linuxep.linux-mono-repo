/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockQueryContext.h"
#include <modules/osqueryextensions/SophosAVDetectionTable.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <gtest/gtest.h>

class TestSophosAVDetectionTable : public LogOffInitializedTests{};

TEST_F(TestSophosAVDetectionTable, expectNoThrowWhenGenerateCalled)
{
    // add generating data required to generate here
    MockQueryContext context;
    EXPECT_NO_THROW(OsquerySDK::SophosAVDetectionTable().Generate(context));
}

//TEST_F(TestSophosAVDetectionTable, testValuesExist)
//{
//    Tests::TempDir tempDir("/tmp");
//    TableRows results;
//    TableRow r;
//    r["time"] = "12000000";
//    r["rowid"] = "1231";
//    r["query_id"] = "reliable";
//    results.push_back(std::move(r));
//    MockQueryContext context;
//    EXPECT_EQ(results,OsquerySDK::SophosAVDetectionTable().Generate(context));
//}