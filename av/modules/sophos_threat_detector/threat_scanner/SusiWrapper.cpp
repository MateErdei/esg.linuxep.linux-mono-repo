/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapper.h"

#include "Logger.h"
#include "SusiGlobalHandler.h"
#include "ThrowIfNotOk.h"

#include <stdexcept>
#include <iostream>

using namespace threat_scanner;

SusiWrapper::SusiWrapper(SusiGlobalHandlerSharePtr globalHandler, const std::string& scannerConfig)
        : m_globalHandler(std::move(globalHandler)),m_handle(nullptr)
{
    auto res = SUSI_CreateScanner(scannerConfig.c_str(), &m_handle);
    throwIfNotOk(res, "Failed to create SUSI Scanner");
    LOGSUPPORT("Creating SUSI scanner");
}

SusiWrapper::~SusiWrapper()
{
    if (m_handle != nullptr)
    {
        auto res = SUSI_DestroyScanner(m_handle);
        throwIfNotOk(res, "Failed to destroy SUSI Scanner");
        LOGTRACE("Destroying SUSI scanner");
    }
}

SusiResult SusiWrapper::scanFile(
        const char* metaData,
        const char* filename,
        datatypes::AutoFd& fd,
        SusiScanResult** scanResult)
{
    SusiResult ret = SUSI_ScanHandle(m_handle, metaData, filename, fd.get(), scanResult);
    return ret;
}

void SusiWrapper::freeResult(SusiScanResult* scanResult)
{
    SUSI_FreeScanResult(scanResult);
}
