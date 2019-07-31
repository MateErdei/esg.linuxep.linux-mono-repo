/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Common::TelemetryConfigImpl
{
    const unsigned int MAX_PORT_NUMBER = 65535;

    const unsigned int MAX_PROCESS_WAIT_RETRIES = 100;
    const unsigned int DEFAULT_PROCESS_WAIT_RETRIES = 10;

    const unsigned int MIN_PROCESS_WAIT_TIMEOUT = 1;      // 1 ms
    const unsigned int MAX_PROCESS_WAIT_TIMEOUT = 600000; // 10 minutes
    const unsigned int DEFAULT_PROCESS_WAIT_TIME = 100;   // 100ms

    const unsigned int MIN_MAX_JSON_SIZE = 10;
    const unsigned int MAX_MAX_JSON_SIZE = 10000000;    // 10MB
    const unsigned int DEFAULT_MAX_JSON_SIZE = 1000000; // 1MB

    const unsigned int MAX_OUTPUT_SIZE = 1000; // 1KB

    const int MIN_PLUGIN_TIMEOUT = 1;                     // 1 ms
    const int MAX_PLUGIN_TIMEOUT = 60000;                 // 1 minute
    const int DEFAULT_PLUGIN_SEND_RECEIVE_TIMEOUT = 5000; // 5 seconds
    const int DEFAULT_PLUGIN_CONNECTION_TIMEOUT = 5000;   // 5 seconds

    const std::string VERB_PUT = "PUT";
    const std::string VERB_GET = "GET";
    const std::string VERB_POST = "POST";
} // namespace Common::TelemetryConfigImpl