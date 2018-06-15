/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "WarehouseError.h"

namespace SulDownloader
{

    std::string toString(WarehouseStatus status)
    {
        switch (status)
        {
            case SUCCESS:
                return "SUCCESS";
            case INSTALLFAILED:
                return "INSTALLFAILED";
            case DOWNLOADFAILED:
                return "DOWNLOADFAILED";
            case RESTARTNEEDED:
                return "RESTARTNEEDED";
            case CONNECTIONERROR:
                return "CONNECTIONERROR";
            case PACKAGESOURCEMISSING:
                return "PACKAGESOURCEMISSING";
            case UNSPECIFIED:
            default:
                return "UNSPECIFIED";

        };

    }
}