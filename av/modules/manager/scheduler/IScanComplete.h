/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

class IScanComplete
{
public:
    virtual ~IScanComplete() = default;
    virtual void processScanComplete(std::string& scanCompletedXml, int exitCode)=0;
};
