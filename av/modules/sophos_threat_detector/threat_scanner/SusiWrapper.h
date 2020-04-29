/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapper.h"
#include "SusiGlobalHandler.h"

#include <memory>
#include <string>

class SusiWrapper : public ISusiWrapper
{
public:
    SusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig);
    ~SusiWrapper();

    SusiResult scanFile(
        const char* metaData,
        const char* filename,
        datatypes::AutoFd& fd,
        SusiScanResult** scanResult) override;

    void freeResult(SusiScanResult* scanResult) override;

private:
    std::unique_ptr<SusiGlobalHandler> m_globalHandler;
    SusiScannerHandle m_handle;

};
