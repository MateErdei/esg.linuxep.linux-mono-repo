/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "gmock/gmock.h"

#include <modules/osqueryclient/IOsqueryClient.h>

#include <string>

using namespace ::testing;

class MockIOsqueryClient : public osqueryclient::IOsqueryClient
{
public:
    MOCK_METHOD1(connect, void(const std::string&));
    MOCK_METHOD2(query, osquery::Status(const std::string&, osquery::QueryData&));
    MOCK_METHOD2(getQueryColumns, osquery::Status(const std::string&, osquery::QueryData&));
};