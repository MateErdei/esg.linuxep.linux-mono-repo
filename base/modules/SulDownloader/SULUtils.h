/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_SULUTILS_H
#define EVEREST_SULUTILS_H
extern "C" {
#include <SUL.h>
}
#include <string>
#include <vector>
#include "WarehouseError.h"


namespace SulDownloader
{

    std::string SulGetErrorDetails( SU_Handle session );
    std::string SulGetLogEntry( SU_Handle session);
    std::string SulQueryProductMetadata( SU_PHandle product, const std::string & attribute, SU_Int index);
    std::string SulQueryDistributionFileData( SU_Handle session, SU_Int index, const std::string & attribute );

    std::pair<WarehouseStatus, std::string > getSulCodeAndDescription( SU_Handle session);

    class SULUtils
    {
    public:
        static bool isSuccess(SU_Result result);
        static void displayLogs(SU_Handle ses);
        static std::vector<std::string> SulLogs(SU_Handle ses);



    };
}

#endif //EVEREST_SULUTILS_H
