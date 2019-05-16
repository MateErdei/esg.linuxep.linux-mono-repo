/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

const unsigned int MAX_PORT_NUMBER = 65535;
const unsigned int MAX_RETRIES = 100;
const unsigned int DEFAULT_RETRIES = 3;

// 10 Minutes
const unsigned int MAX_TIMEOUT = 600000;
// 5 Seconds
const unsigned int DEFAULT_TIMEOUT = 5000;

// 10MB
const unsigned int MAX_MAX_JSON_SIZE = 10000000;

// 10B
const unsigned int MIN_MAX_JSON_SIZE = 10;

// 1MB
const unsigned int DEFAULT_MAX_JSON_SIZE = 1000000;

const std::string DEFAULT_VERB = "PUT";