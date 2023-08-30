// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>
namespace Common
{
    namespace DownloadReport
    {
        /**
         * Status to enable reporting the expected Message Numbers of ALC events.
         * https://wiki.sophos.net/pages/viewpage.action?spaceKey=SophosCloud&title=EMP%3A+event-alc
         */
        enum RepositoryStatus
        {
            SUCCESS = 0,
            INSTALLFAILED = 103,
            DOWNLOADFAILED = 107,
            RESTARTNEEDED = 109,
            CONNECTIONERROR = 112,
            PACKAGESOURCEMISSING = 111,
            UNINSTALLFAILED = 120,
            UNSPECIFIED = 121
        };

        /**
         * Allow DownloadReport to create JSON files from the WarehouseStatus.
         * @return String with the name of the enum status.
         */
        std::string toString(RepositoryStatus);

        /**
         * Allow DownloadReport to load back the WarehouseStatus from the string representation.
         * @param serializedStatus
         * @param status: passed as pointer to make it clear in the calling code that it is an input.
         */
        void fromString(const std::string& serializedStatus, RepositoryStatus* status);
    } // namespace DownloadReport

} // namespace Common
