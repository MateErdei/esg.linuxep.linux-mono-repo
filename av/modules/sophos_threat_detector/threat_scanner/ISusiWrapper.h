/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/AutoFd.h"

#include <Susi.h>

#include <memory>

namespace threat_scanner
{
    class ISusiWrapper
    {
    public:
        virtual SusiResult scanFile(
            const char* metaData,
            const char* filename,
            datatypes::AutoFd& fd,
            SusiScanResult** scanResult) = 0;

        virtual void freeResult(SusiScanResult* scanResult) = 0;

        virtual bool update(const std::string& path) = 0;

        virtual ~ISusiWrapper() = default;
    };
    using ISusiWrapperSharedPtr = std::shared_ptr<ISusiWrapper>;
}
