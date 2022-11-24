// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/RestoreReport.h"

class IRestoreReportProcessor
{
public:
    virtual ~IRestoreReportProcessor() = default;
    virtual void processRestoreReport(const scan_messages::RestoreReport& restoreReport) const = 0;
};
