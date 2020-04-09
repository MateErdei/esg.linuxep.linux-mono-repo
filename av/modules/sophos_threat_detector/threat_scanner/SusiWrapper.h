/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SusiGlobalHandler.h"

#include <Susi.h>

#include <memory>
#include <string>

class SusiWrapper
{
public:
    SusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig);
    ~SusiWrapper();

    SusiScannerHandle getHandle();

private:
    std::unique_ptr<SusiGlobalHandler> m_globalHandler;
    SusiScannerHandle m_handle;

};
