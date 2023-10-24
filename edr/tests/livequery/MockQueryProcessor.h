// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "livequery/IQueryProcessor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockQueryProcessor : public virtual livequery::IQueryProcessor
{
public:
    MOCK_METHOD1(query, livequery::QueryResponse(const std::string&));
    MOCK_METHOD0(clone, std::unique_ptr<livequery::IQueryProcessor>());
};
