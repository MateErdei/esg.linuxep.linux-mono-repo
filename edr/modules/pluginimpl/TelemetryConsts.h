/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

namespace plugin
{
    const char* const telemetryOsqueryRestarts = "osquery-restarts";
    const char* const version = "version";
    const char* const telemetryOSQueryRestartsCPU = "osquery-restarts-cpu";
    const char* const telemetryOSQueryRestartsMemory = "osquery-restarts-memory";
    const char* const telemetryOSQueryDatabaseSize = "osquery-database-size";
    const char* const telemetryMTRExtensionRestarts = "mtr-extension-restarts";
    // the following two variables are also defined in osqueryextensions to avoid circular dependencies
    const char* const telemetrySophosExtensionRestarts = "sophos-extension-restarts";
    const char* const telemetryLoggerExtensionRestarts = "logger-extension-restarts";
    const char* const telemetryMTRExtensionRestartsCPU = "mtr-extension-restarts-cpu";
    const char* const telemetryMTRExtensionRestartsMemory = "mtr-extension-restarts-memory";
    const char* const telemetrySuccessfulQueries = "successful-count";
    const char* const telemetryFailedQueriesOsqueryError = "failed-osquery-error-count";
    const char* const telemetryFailedQueriesLimitExceeded = "failed-exceed-limit-count";
    const char* const telemetryFailedQueriesOsqueryDied = "failed-osquery-died-count";
    const char* const telemetryFailedQueriesUnexpected = "failed-unexpected-error-count";
    const char* const telemetryIsXdrEnabled = "xdr-is-enabled";
    const char* const telemetryEventsMax = "events-max";
    const char* const telemetryScheduledQueries = "scheduled-queries";
    const char* const telemetryQueryErrorCount = "query-error-count";
    const char* const telemetryRecordSize = "record-size";
    const char* const telemetryRecordsCount = "records-count";
    const char* const telemetryProcessEventsMaxHit = "reached-max-process-events";
    const char* const telemetrySyslogEventsMaxHit = "reached-max-syslog-events";
    const char* const telemetrySocketEventsMaxHit = "reached-max-socket-events";
    const char* const telemetryUserEventsMaxHit = "reached-max-user-events";
    const char* const telemetrySelinuxEventsMaxHit = "reached-max-selinux-events";
    const char* const telemetryFoldableQueries = "foldable-queries";
    const char* const telemetryFoldedCount = "folded-count";
}