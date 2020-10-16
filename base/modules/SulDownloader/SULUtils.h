/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <SulDownloader/suldownloaderdata/WarehouseError.h>

extern "C"
{
#include <SUL.h>
}

#include <string>
#include <vector>

namespace SulDownloader
{
    std::string SulGetErrorDetails(SU_Handle session);
    std::string SulGetLogEntry(SU_Handle session);
    std::string SulQueryProductMetadata(SU_PHandle product, const std::string& attribute, SU_Int index);
    bool SulSetLanguage(SU_Handle session, SU_ConstString language);

    std::pair<suldownloaderdata::WarehouseStatus, std::string> getSulCodeAndDescription(SU_Handle session);

    class SULUtils
    {
    public:
        using SulLogsVector = std::vector<std::string>;
        static bool isSuccess(SU_Result result);
        static void displayLogs(SU_Handle ses, SulLogsVector& sulLogs);
        static SulLogsVector SulLogs(SU_Handle ses);
    };
} // namespace SulDownloader
