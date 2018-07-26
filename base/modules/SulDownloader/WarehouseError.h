/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>
namespace SulDownloader
{

     /**
      * Status to enable reporting the expected Message Numbers of ALC events.
      * https://wiki.sophos.net/pages/viewpage.action?spaceKey=SophosCloud&title=EMP%3A+event-alc
      */
    enum WarehouseStatus{ SUCCESS=0, INSTALLFAILED=-1, DOWNLOADFAILED=-2, RESTARTNEEDED=-3, CONNECTIONERROR=-4, PACKAGESOURCEMISSING=-5, UNSPECIFIED=-6 };

    /**
     * Allow DownloadReport to create JSON files from the WarehouseStatus.
     * @return String with the name of the enum status.
     */
    std::string toString( WarehouseStatus );

    /**
     * Struct to keep track of report of failures both of the SUL library and the SULDownloader.
     *
     */
    struct WarehouseError
    {
        std::string Description; /// Description of failure provided by SULDownloader
        std::string SulError; /// Description of failure provided by SUL library.
        WarehouseStatus  status;
    };

}




