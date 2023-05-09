// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "unixsocket/restoreReportingSocket/IRestoreReportingClient.h"

#include <gmock/gmock.h>

class MockRestoreReportingClient : public unixsocket::IRestoreReportingClient
{
public:
    MOCK_METHOD(void, sendRestoreReport, (const scan_messages::RestoreReport& restoreReport), (override));
};
