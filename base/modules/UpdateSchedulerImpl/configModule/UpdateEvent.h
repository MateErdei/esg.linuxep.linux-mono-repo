// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/UtilityImpl/TimeUtils.h"
#include "UpdateScheduler/IMapHostCacheId.h"
#include "UpdateSchedulerImpl/common/EventMessageNumber.h"

#include <string>
#include <vector>

namespace UpdateSchedulerImpl::configModule
{
    struct MessageInsert
    {
        MessageInsert(std::string pN, std::string err) : PackageName(std::move(pN)), ErrorDetails(std::move(err)) {}

        std::string PackageName;
        std::string ErrorDetails;
    };

    struct UpdateEvent
    {
        UpdateEvent() : IsRelevantToSend(false), MessageNumber(EventMessageNumber::SUCCESS), Messages(), UpdateSource()
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
} // namespace UpdateSchedulerImpl::configModule
