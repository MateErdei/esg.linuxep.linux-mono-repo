/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISusiWrapper.h"
#include "SusiGlobalHandler.h"

#include <memory>
#include <string>

namespace threat_scanner
{
    class SusiWrapper : public ISusiWrapper
    {
    public:
        SusiWrapper(SusiGlobalHandlerSharedPtr globalHandler, const std::string& scannerConfig);
        ~SusiWrapper() override;

        SusiResult scanFile(
            const char* metaData,
            const char* filename,
            datatypes::AutoFd& fd,
            SusiScanResult** scanResult) override;

        void freeResult(SusiScanResult* scanResult) override;

    private:
        SusiGlobalHandlerSharedPtr m_globalHandler;
        SusiScannerHandle m_handle;
    };
}
