/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

namespace plugin
{
    const char* const telemetryOsqueryRestarts = "osquery-restarts";
    const char* const version = "version";
    const char* const telemetryOSQueryRestartsCPU = "osquery-restarts-cpu";
    const char* const telemetryOSQueryRestartsMemory = "osquery-restarts-memory";
    const char* const telemetryOSQueryDatabasePurges = "osquery-database-purges";
    const char* const telemetryOSQueryDatabaseSize = "osquery-database-size";
    const char* const telemetrySuccessfulQueries = "livequery-successful-queries";
    const char* const telemetryFailedQueriesOsqueryError = "livequery-failed-osquery-error";
    const char* const telemetryFailedQueriesLimitExceeded = "livequery-failed-limit-exceeded";
    const char* const telemetryFailedQueriesOsqueryDied = "livequery-failed-osquery-died";
    const char* const telemetryFailedQueriesUnexpected = "livequery-failed-unexpected";
}