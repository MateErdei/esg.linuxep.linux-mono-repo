// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

namespace UpdateSchedulerImpl
{
    enum EventMessageNumber
    {
        SUCCESS = 0,
        INSTALLFAILED = 103,      /* IDS_AUERROR_INSTALL "Failed to install %1: %2" */
        INSTALLCAUGHTERROR = 106, /* IDS_AUERROR_GENERAL "Installation caught error %1" */
        DOWNLOADFAILED = 107,     /* IDS_AUERROR_DOWNLOAD "Download of %1 failed from server %2" */
        UPDATECANCELLED = 108,    /* IDS_AUERROR_CANCEL "The update was cancelled by the user" */
        RESTARTEDNEEDED = 109,    /* IDS_AUERROR_REBOOTREQUIRED "Restart needed for updates to take effect" */
        UPDATESOURCEMISSING =
            110, /* IDS_AUERROR_NOTCONFIGURED "Updating failed because no update source has been specified" */
        SINGLEPACKAGEMISSING =
            111,               /* IDS_AUERROR_SOURCENOTFOUND "ERROR: Could not find a source for updated package %1" */
        CONNECTIONERROR = 112, /* IDS_AUERROR_CONNECTIONFAILED "There was a problem while establishing a connection to
                                  the server. Details: %1" */
        MULTIPLEPACKAGEMISSING = 113
    };
}
