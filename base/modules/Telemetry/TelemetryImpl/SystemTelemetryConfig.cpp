/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemTelemetryConfig.h"

namespace Telemetry
{
    const SystemTelemetryConfig GL_systemTelemetryObjectsConfig = {
        { "kernel",
          SystemTelemetryTuple{ "/usr/bin/hostnamectl",
                                "",
                                "^\\s*Kernel:\\s*(.*)$",
                                { { "", TelemetryValueType::STRING } } } },
        { "os-name",
          SystemTelemetryTuple{ "/usr/bin/head",
                                "-100 /etc/os-release", // only first dozen or so lines needed or expected
                                "^NAME=\"([^\"]*).*$",
                                { { "", TelemetryValueType::STRING } } } },
        { "os-version",
          SystemTelemetryTuple{ "/usr/bin/head",
                                "-100 /etc/os-release", // only first dozen or so lines needed or expected
                                "^VERSION_ID=\"([^\"]*).*$",
                                { { "", TelemetryValueType::STRING } } } },
        { "cpu-cores",
          SystemTelemetryTuple{ "/usr/bin/lscpu",
                                "",
                                "^CPU\\(s\\):\\s*(\\d+)$",
                                { { "", TelemetryValueType::INTEGER } } } },
        { "memory-total",
          SystemTelemetryTuple{ "/usr/bin/free",
                                "-t",
                                "^Total:\\s*(\\d+).*$",
                                { { "", TelemetryValueType::INTEGER } } } },
        { "locale",
          SystemTelemetryTuple{ "/usr/bin/localectl",
                                "",
                                "\\s*System Locale:\\s*LANG=\\s*(.*)$",
                                { { "", TelemetryValueType::STRING } } } },
        { "uptime",
          SystemTelemetryTuple{ "/usr/bin/uptime",
                                "--pretty",
                                "^up\\s*(.*)$",
                                { { "", TelemetryValueType::STRING } } } }
    };

    const SystemTelemetryConfig GL_systemTelemetryArraysConfig = {
        { "disks",
          SystemTelemetryTuple{ "/bin/df",
                                "-T --local",
                                "^\\s*\\S+\\s+(\\S+)\\s+\\S+\\s+\\S+\\s+(\\d+)\\s*.*$",
              { { "fstype", TelemetryValueType::STRING }, { "free", TelemetryValueType::INTEGER } } } }
    };
} // namespace Telemetry
