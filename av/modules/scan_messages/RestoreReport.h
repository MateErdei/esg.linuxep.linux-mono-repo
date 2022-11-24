// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "Common/UtilityImpl/Uuid.h"

#include <RestoreReport.capnp.h>

#include <capnp/message.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace scan_messages
{
    struct RestoreReport
    {
        RestoreReport(std::int64_t time, std::string path, std::string correlationId, bool wasSuccessful) :
            time(time), path(std::move(path)), correlationId(std::move(correlationId)), wasSuccessful(wasSuccessful)
        {
        }

        explicit RestoreReport(const Sophos::ssplav::RestoreReport::Reader& reader) :
            RestoreReport(reader.getTime(), reader.getPath(), reader.getCorrelationId(), reader.getWasSuccessful())
        {
        }

        [[nodiscard]] std::unique_ptr<::capnp::MallocMessageBuilder> serialise() const
        {
            auto message = std::make_unique<::capnp::MallocMessageBuilder>();

            Sophos::ssplav::RestoreReport::Builder restoreReportBuilder =
                message->initRoot<Sophos::ssplav::RestoreReport>();

            restoreReportBuilder.setTime(time);
            restoreReportBuilder.setPath(path);
            restoreReportBuilder.setCorrelationId(correlationId);
            restoreReportBuilder.setWasSuccessful(wasSuccessful);

            return message;
        }

        void validate() const
        {
            if (path.find('\0') != std::string::npos)
            {
                throw std::runtime_error("Path contains null character");
            }
            if (!Common::UtilityImpl::Uuid::IsValid(correlationId))
            {
                throw std::runtime_error("Correlation ID is not a valid UUID");
            }
        }

        bool operator==(const RestoreReport& other) const
        {
            return time == other.time && path == other.path && correlationId == other.correlationId &&
                   wasSuccessful == other.wasSuccessful;
        }

        std::int64_t time;
        std::string path;
        std::string correlationId;
        bool wasSuccessful;
    };
} // namespace scan_messages
