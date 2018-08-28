/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <SulDownloader/suldownloaderdata/WarehouseError.h>

extern "C" {
#include <SUL.h>
}

#include <string>
#include <vector>

namespace SulDownloader
{

    std::string SulGetErrorDetails( SU_Handle session );
    std::string SulGetLogEntry( SU_Handle session);
    std::string SulQueryProductMetadata( SU_PHandle product, const std::string & attribute, SU_Int index);

    std::pair<suldownloaderdata::WarehouseStatus, std::string > getSulCodeAndDescription( SU_Handle session);

    class SULUtils
    {
    public:
        static bool isSuccess(SU_Result result);
        static void displayLogs(SU_Handle ses);
        static std::vector<std::string> SulLogs(SU_Handle ses);



    };
}


