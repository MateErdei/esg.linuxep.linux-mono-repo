/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <OsquerySDK/OsquerySDK.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace OsquerySDK;

class MockQueryContext : public virtual QueryContextInterface
{
public:
    MOCK_METHOD1(Matches, bool(TableRow&));
    MOCK_METHOD2(Matches, bool(const std::string&,const std::string&));
    MOCK_METHOD1(IsColumnUsed, bool(const std::string&));
    MOCK_METHOD1(IsConstraintUsed, bool(const std::string&));
    MOCK_METHOD1(CountConstraints, size_t(const std::string&));
    MOCK_METHOD0(GetUsedColumns, UsedColumns&());
    MOCK_METHOD0(GetAllConstraints, ConstraintMap&());
    MOCK_METHOD2(GetConstraints, std::set<std::string>(const std::string&, ConstraintOperator));
};
