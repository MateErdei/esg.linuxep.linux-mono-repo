/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapper.h"

#include "Logger.h"
#include "SusiGlobalHandler.h"

#include <stdexcept>
#include <iostream>

void susiLogCallback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    if (!m.empty())
    {
        LOGERROR(level << ": " << m);
    }
}

static const SusiLogCallback GL_log_callback{
        .version = SUSI_LOG_CALLBACK_VERSION,
        .token = nullptr,
        .log = susiLogCallback,
        .minLogLevel = SUSI_LOG_LEVEL_DETAIL
};

static void throwIfNotOk(SusiResult res, const std::string& message)
{
    if (res != SUSI_S_OK)
    {
        throw std::runtime_error(message);
    }
}

SusiWrapper::SusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig)
        : m_handle(nullptr)
{
    SusiResult res = SUSI_SetLogCallback(&GL_log_callback);
    throwIfNotOk(res, "Failed to set log callback");

    m_globalHandler = std::make_unique<SusiGlobalHandler>(runtimeConfig);

    res = SUSI_CreateScanner(scannerConfig.c_str(), &m_handle);
    throwIfNotOk(res, "Failed to create SUSI Scanner");
    LOGINFO("Susi scanner constructed");
}

SusiWrapper::~SusiWrapper()
{
    if (m_handle != nullptr)
    {
        SusiResult res = SUSI_DestroyScanner(m_handle);
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