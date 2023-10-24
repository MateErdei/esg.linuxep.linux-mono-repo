// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

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
    MOCK_METHOD0(CountAllConstraints, size_t());
    MOCK_METHOD0(GetUsedColumns, UsedColumns&());
    MOCK_METHOD0(GetAllConstraints, ConstraintMap&());
    MOCK_METHOD2(GetConstraints, std::set<std::string>(const std::string&, ConstraintOperator));
    MOCK_METHOD3(AddConstraint, void(const std::string&, ConstraintOperator, const std::string&));
};
