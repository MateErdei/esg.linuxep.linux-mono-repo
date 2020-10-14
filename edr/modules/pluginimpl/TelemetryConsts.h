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
    const char* const telemetrySuccessfulQueries = "successful-count";
    const char* const telemetryFailedQueriesOsqueryError = "failed-osquery-error-count";
    const char* const telemetryFailedQueriesLimitExceeded = "failed-exceed-limit-count";
    const char* const telemetryFailedQueriesOsqueryDied = "failed-osquery-died-count";
    const char* const telemetryFailedQueriesUnexpected = "failed-unexpected-error-count";
    const char* const telemetryIsXdrEnabled = "xdr-is-enabled";
}