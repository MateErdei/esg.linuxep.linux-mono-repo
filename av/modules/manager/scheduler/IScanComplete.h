/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

class IScanComplete
{
public:
    virtual ~IScanComplete() = default;
    virtual void processScanComplete(std::string& scanCompletedXml)=0;
};
