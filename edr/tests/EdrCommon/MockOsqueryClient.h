/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "gmock/gmock.h"

#include "osqueryclient/IOsqueryClient.h"

#include <string>

using namespace ::testing;

class MockIOsqueryClient : public osqueryclient::IOsqueryClient
{
public:
    MOCK_METHOD1(connect, void(const std::string&));
    MOCK_METHOD2(query, OsquerySDK::Status(const std::string&, OsquerySDK::QueryData&));
    MOCK_METHOD2(getQueryColumns, OsquerySDK::Status(const std::string&, OsquerySDK::QueryColumns&));
};