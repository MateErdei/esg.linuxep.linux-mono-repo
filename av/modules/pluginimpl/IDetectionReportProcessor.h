// Copyright 2022, Sophos Limited.  All rights reserved.
#pragma once

#include "../scan_messages/ServerThreatDetected.h"


class IDetectionReportProcessor
{
public:
    virtual ~IDetectionReportProcessor() = default;
    virtual void processDetectionReport(const scan_messages::ServerThreatDetected&)=0;
};

