// Copyright 2021 Sophos Limited. All rights reserved.

#pragma once

namespace JournalerCommon
{
    namespace Telemetry
    {
        const char* const version = "version";
        const char* const telemetryDroppedAvEvents = "dropped-av-events";
        const char* const telemetryFailedEventWrites = "failed-event-writes";
        const char* const telemetryMissingEventSubscriberSocket = "event-subscriber-socket-missing";
        const char* const telemetryThreadHealthPrepender = "thread-health.";
        const char* const telemetryAcceptableDroppedEventsExceeded = "acceptable-daily-dropped-events-exceeded";
        const char* const telemetryAttemptedJournalWrites = "attempted-journal-writes";
        const char* const pluginHealthStatus = "health";
    } // namespace Telemetry
} // namespace JournalerCommon