/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapper.h"

#include "Logger.h"
#include "SusiGlobalHandler.h"

#include <stdexcept>
#include <iostream>

using namespace threat_scanner;

static void throwIfNotOk(SusiResult res, const std::string& message)
{
    if (res != SUSI_S_OK)
    {
        throw std::runtime_error(message);
    }
}

SusiWrapper::SusiWrapper(SusiGlobalHandlerSharePtr globalHandler, const std::string& scannerConfig)
        : m_globalHandler(std::move(globalHandler)),m_handle(nullptr)
{
    auto res = SUSI_CreateScanner(scannerConfig.c_str(), &m_handle);
    throwIfNotOk(res, "Failed to create SUSI Scanner");
    LOGINFO("Susi scanner constructed");
}

SusiWrapper::~SusiWrapper()
{
    if (m_handle != nullptr)
    {
        auto res = SUSI_DestroyScanner(m_handle);
        throwIfNotOk(res, "Failed to destroy SUSI Scanner");
        LOGINFO("Susi scanner destroyed");
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