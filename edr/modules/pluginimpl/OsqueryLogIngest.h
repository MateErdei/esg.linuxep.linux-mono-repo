/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>

class OsqueryLogIngest
{
public:
    void operator()(std::string output);
    std::tuple<bool, std::string> processOsqueryLogLineForScheduledQueries(std::string& logLine);
    bool isGenericLogLine(std::string& logLine);
private:
    /// Logging
    ///   Takes in OUTPUT_BUFFER_LIMIT_BYTES number of bytes from the output of the osquery watchdog which
    ///   doesn't seem to have an option to log to a file. For every line it receives it logs the line in the EDR log
    ///   using the separate osquery logger so we can distinguish between EDR log lines and osqueryd log lines.
    /// Telemetry
    ///   The output from osquery is used to check for osquery restarts due to CPU and memory.
    /// \param output OUTPUT_BUFFER_LIMIT_BYTES number of bytes from osquery output.
    void ingestOutput(const std::string& output);
    void processOsqueryLogLineForTelemetry(std::string& logLine);
    bool processOsqueryLogLineForEventsMaxTelemetry(std::string& logLine);
    std::string m_previousSection;
};


