/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace Telemetry::TelemetryConfigImpl
{
    const unsigned int MAX_PORT_NUMBER = 65535;

    const unsigned int MAX_PROCESS_WAIT_RETRIES = 100;
    const unsigned int DEFAULT_PROCESS_WAIT_RETRIES = 10;

    const unsigned int MAX_PROCESS_WAIT_TIMEOUT = 600000; // 10 minutes
    const unsigned int DEFAULT_PROCESS_WAIT_TIME = 100;   // 100 ms

    const unsigned int MAX_MAX_JSON_SIZE = 10000000; // 10MB
    const unsigned int MIN_MAX_JSON_SIZE = 10;
    const unsigned int DEFAULT_MAX_JSON_SIZE = 1000000; // 1MB

    const std::string VERB_PUT = "PUT";
    const std::string VERB_GET = "GET";
    const std::string VERB_POST = "POST";
}