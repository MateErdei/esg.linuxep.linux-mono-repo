/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Susi.h>

class ISusiWrapper
{
public:
    virtual SusiResult scanFile(
            const char* metaData,
            const char* filename,
            SusiScanResult** scanResult) = 0;

    virtual void freeResult(SusiScanResult* scanResult) = 0;
};