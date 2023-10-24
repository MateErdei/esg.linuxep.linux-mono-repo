// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "livequery/IResponseDispatcher.h"
#include "livequery/QueryResponse.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockResponseDispatcher : public virtual livequery::IResponseDispatcher
{
public:
    MOCK_METHOD2(sendResponse, void(const std::string&, const livequery::QueryResponse& ));
    MOCK_METHOD0(clone, std::unique_ptr<livequery::IResponseDispatcher>());
    MOCK_METHOD1(setTelemetry, void(const std::string&));
    MOCK_METHOD0(getTelemetry, std::string(void));
};
