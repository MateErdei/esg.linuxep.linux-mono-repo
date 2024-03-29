// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "RepositoryStatus.h"

std::string Common::DownloadReport::toString(RepositoryStatus status)
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
        case UNINSTALLFAILED:
            return "UNINSTALLFAILED";
        case UNSPECIFIED:
        default:
            return "UNSPECIFIED";
    }
}

void Common::DownloadReport::fromString(const std::string& serializedStatus, RepositoryStatus* status)
{
    if (serializedStatus == "SUCCESS")
    {
        *status = SUCCESS;
    }
    else if (serializedStatus == "INSTALLFAILED")
    {
        *status = INSTALLFAILED;
    }
    else if (serializedStatus == "DOWNLOADFAILED")
    {
        *status = DOWNLOADFAILED;
    }
    else if (serializedStatus == "RESTARTNEEDED")
    {
        *status = RESTARTNEEDED;
    }
    else if (serializedStatus == "CONNECTIONERROR")
    {
        *status = CONNECTIONERROR;
    }
    else if (serializedStatus == "PACKAGESOURCEMISSING")
    {
        *status = PACKAGESOURCEMISSING;
    }
    else if (serializedStatus == "UNINSTALLFAILED")
    {
        *status = UNINSTALLFAILED;
    }
    else
    {
        *status = UNSPECIFIED;
    }
}
