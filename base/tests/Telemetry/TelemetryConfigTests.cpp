/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

//#include "MockHttpSender.h"
//#include "MockTelemetryProvider.h"

//#include <Telemetry/TelemetryImpl/Telemetry.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Telemetry/TelemetryConfig/Config.h>
#include <modules/Telemetry/TelemetryConfig/TelemetryConfigSerialiser.h>
//#include "../../thirdparty/nlohmann-json/json.hpp"

//#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
//#include <modules/Telemetry/TelemetryImpl/TelemetryProcessor.h>
//#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
//#include <tests/Common/Helpers/MockFileSystem.h>

using ::testing::StrictMock;
using namespace Telemetry;



TEST(TelemetryConfigTest, serialiseTelemetryConfig) // NOLINT
{
    Telemetry::TelemetryConfig::Config config;
    config.m_server = "";
    config.m_verb  = Telemetry::TelemetryConfig::Config::HttpVerb::GET;
    config.m_externalProcessTimeout = 3;
    config.m_headers = {"header1", "header2"};
    config.m_maxJsonSize = 10;
    config.m_port = 123;
    config.m_resourceRoute = "/dev";

    Telemetry::TelemetryConfig::MessageRelay messageRelay;
    messageRelay.m_url = "relay";
    messageRelay.m_port = 456;
    messageRelay.m_authentication = Telemetry::TelemetryConfig::MessageRelay::Authentication::basic;
    messageRelay.m_certificatePath = "certpath";
    messageRelay.m_username = "relayuser";
    messageRelay.m_password = "relaypw";

    config.m_messageRelays.push_back(messageRelay);

    Telemetry::TelemetryConfig::Proxy proxy;
    proxy.m_url = "proxy";
    proxy.m_port = 789;
    proxy.m_authentication = Telemetry::TelemetryConfig::MessageRelay::Authentication::basic;
    proxy.m_username = "proxyuser";
    proxy.m_password = "proxypw";

    config.m_proxies.push_back(proxy);

    //Telemetry::TelemetryConfig::TelemetryConfigSerialiser serialiser;
    //serialiser
}

