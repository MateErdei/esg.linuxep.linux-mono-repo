/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

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
            INSTALLFAILED = 103,
            INSTALLCAUGHTERROR = 106,
            DOWNLOADFAILED = 107,
            UPDATECANCELLED = 108,
            RESTARTEDNEEDED = 109,
            UPDATESOURCEMISSING = 110,
            SINGLEPACKAGEMISSING = 111,
            CONNECTIONERROR = 112,
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
