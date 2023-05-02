/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

namespace plugin
{
    const char* const telemetryOsqueryRestarts = "osquery-restarts";
    const char* const version = "version";
    const char* const health = "health";
    const char* const telemetryOSQueryRestartsCPU = "osquery-restarts-cpu";
    const char* const telemetryOSQueryRestartsMemory = "osquery-restarts-memory";
    const char* const telemetryOSQueryDatabaseSize = "osquery-database-size";
    const char* const telemetrySuccessfulQueries = "successful-count";
    const char* const telemetryFailedQueries = "failed-count";

    const char* const telemetryIsXdrEnabled = "xdr-is-enabled";
    const char* const telemetryEventsMax = "events-max";
    const char* const telemetryScheduledQueries = "scheduled-queries";
    const char* const telemetryUploadLimitHitQueries = "upload-limit-hit";
    const char* const telemetryLiveQueries = "live-query";
    const char* const telemetryProcessEventsMaxHit = "reached-max-process-events";
    const char* const telemetrySyslogEventsMaxHit = "reached-max-syslog-events";
    const char* const telemetrySocketEventsMaxHit = "reached-max-socket-events";
    const char* const telemetryUserEventsMaxHit = "reached-max-user-events";
    const char* const telemetrySelinuxEventsMaxHit = "reached-max-selinux-events";
    const char* const telemetryFoldableQueries = "foldable-queries";

    const char* const telemetryEdrRestartsMemory = "edr-restarts-memory";
}