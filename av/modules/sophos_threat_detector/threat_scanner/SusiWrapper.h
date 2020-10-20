/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

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
        SusiWrapper(SusiGlobalHandlerSharePtr globalHandler, const std::string& scannerConfig);
        ~SusiWrapper();

        SusiResult scanFile(
            const char* metaData,
            const char* filename,
            datatypes::AutoFd& fd,
            SusiScanResult** scanResult) override;

        void freeResult(SusiScanResult* scanResult) override;

        bool update(const std::string& path) override;

    private:
        SusiGlobalHandlerSharePtr m_globalHandler;
        SusiScannerHandle m_handle;
    };
}
