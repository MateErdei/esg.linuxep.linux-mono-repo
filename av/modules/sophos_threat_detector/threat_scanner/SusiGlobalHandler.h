/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

class SusiGlobalHandler
{
public:
    explicit SusiGlobalHandler(const std::string& json_config);
    ~SusiGlobalHandler();
};