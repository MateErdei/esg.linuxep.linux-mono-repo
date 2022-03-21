/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapper.h"

#include "Logger.h"
#include "SusiGlobalHandler.h"
#include "ThrowIfNotOk.h"
#include "common/ShuttingDownException.h"

#include <stdexcept>
#include <iostream>

namespace threat_scanner
{
    SusiWrapper::SusiWrapper(SusiGlobalHandlerSharedPtr globalHandler, const std::string& scannerConfig) :
        m_globalHandler(std::move(globalHandler)), m_handle(nullptr)
    {
        LOGSUPPORT("Creating SUSI scanner");
        auto res = SUSI_CreateScanner(scannerConfig.c_str(), &m_handle);
        throwIfNotOk(res, "Failed to create SUSI Scanner");
        LOGSUPPORT("Created SUSI scanner");
    }

    SusiWrapper::~SusiWrapper()
    {
        if (m_handle != nullptr)
        {
            LOGSUPPORT("Destroying SUSI scanner");
            auto res = SUSI_DestroyScanner(m_handle);
            throwIfNotOk(res, "Failed to destroy SUSI Scanner");
            LOGSUPPORT("Destroyed SUSI scanner");
        }
    }

    SusiResult SusiWrapper::scanFile(
        const char* metaData,
        const char* filename,
        datatypes::AutoFd& fd,
        SusiScanResult** scanResult)
    {
        if (m_globalHandler->isShuttingDown())
        {
            throw ShuttingDownException();
        }

        SusiResult ret = SUSI_ScanHandle(m_handle, metaData, filename, fd.get(), scanResult);
        return ret;
    }

    void SusiWrapper::freeResult(SusiScanResult* scanResult)
    {
        SUSI_FreeScanResult(scanResult);
    }
} // namespace threat_scanner