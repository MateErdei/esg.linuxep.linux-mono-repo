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
        struct MessageInsert
        {
            MessageInsert(std::string pN, std::string err) : PackageName(std::move(pN)), ErrorDetails(std::move(err)) {}

            std::string PackageName;
            std::string ErrorDetails;
        };

        struct UpdateEvent
        {
            UpdateEvent() : IsRelevantToSend(false), MessageNumber(0), Messages(), UpdateSource() {}

            bool IsRelevantToSend;
            int MessageNumber;
            std::vector<MessageInsert> Messages;
            std::string UpdateSource;
        };

        std::string serializeUpdateEvent(
            const UpdateEvent& updateEvent,
            const UpdateScheduler::IMapHostCacheId& iMapHostCacheId,
            const Common::UtilityImpl::IFormattedTime& iFormattedTime);
    } // namespace configModule

} // namespace UpdateSchedulerImpl
