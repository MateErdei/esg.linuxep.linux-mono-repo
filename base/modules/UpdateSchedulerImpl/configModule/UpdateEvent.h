/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/UtilityImpl/TimeUtils.h>
#include <UpdateScheduler/IMapHostCacheId.h>

#include <string>
#include <vector>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        enum EventMessageNumber
        {
            SUCCESS = 0,
            INSTALLFAILED = 103,            /* IDS_AUERROR_INSTALL "Failed to install %1: %2" */
            INSTALLCAUGHTERROR = 106,       /* IDS_AUERROR_GENERAL "Installation caught error %1" */
            DOWNLOADFAILED = 107,           /* IDS_AUERROR_DOWNLOAD "Download of %1 failed from server %2" */
            UPDATECANCELLED = 108,          /* IDS_AUERROR_CANCEL "The update was cancelled by the user" */
            RESTARTEDNEEDED = 109,          /* IDS_AUERROR_REBOOTREQUIRED "Restart needed for updates to take effect" */
            UPDATESOURCEMISSING = 110,      /* IDS_AUERROR_NOTCONFIGURED "Updating failed because no update source has been specified" */
            SINGLEPACKAGEMISSING = 111,     /* IDS_AUERROR_SOURCENOTFOUND "ERROR: Could not find a source for updated package %1" */
            CONNECTIONERROR = 112,          /* IDS_AUERROR_CONNECTIONFAILED "There was a problem while establishing a connection to the server. Details: %1" */
            MULTIPLEPACKAGEMISSING = 113
        };

        struct MessageInsert
        {
            MessageInsert(std::string pN, std::string err) : PackageName(std::move(pN)), ErrorDetails(std::move(err)) {}

            std::string PackageName;
            std::string ErrorDetails;
        };

        struct UpdateEvent
        {
            UpdateEvent() :
                IsRelevantToSend(false),
                MessageNumber(EventMessageNumber::SUCCESS),
                Messages(),
                UpdateSource()
            {
            }

            bool IsRelevantToSend;
            EventMessageNumber MessageNumber;
            std::vector<MessageInsert> Messages;
            std::string UpdateSource;
        };

        std::string serializeUpdateEvent(
            const UpdateEvent& updateEvent,
            const UpdateScheduler::IMapHostCacheId& iMapHostCacheId,
            const Common::UtilityImpl::IFormattedTime& iFormattedTime);
    } // namespace configModule

} // namespace UpdateSchedulerImpl
