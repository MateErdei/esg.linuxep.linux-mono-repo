/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SystemTelemetryConfig.h"

namespace Telemetry
{
    const SystemTelemetryConfig GL_systemTelemetryObjectsConfig = {
        { "kernel",
          SystemTelemetryTuple{ "hostnamectl",
                                {},
                                R"(^\s*Kernel:\s*(.*)$)",
                                { { "", TelemetryValueType::STRING } } } },
        { "os-name",
          SystemTelemetryTuple{ "head",
                                { "-100", "/etc/os-release" }, // only first dozen or so lines needed or expected
                                R"(^NAME="([^"]*).*$)",
                                { { "", TelemetryValueType::STRING } } } },
        { "os-version",
          SystemTelemetryTuple{ "head",
                                { "-100", "/etc/os-release" }, // only first dozen or so lines needed or expected
                                R"(^VERSION_ID="([^"]*).*$)",
                                { { "", TelemetryValueType::STRING } } } },
        { "cpu-cores",
          SystemTelemetryTuple{ "lscpu",
                                {},
                                R"(^CPU\(s\):\s*(\d+)$)",
                                { { "", TelemetryValueType::INTEGER } } } },
        { "memory-total",
          SystemTelemetryTuple{ "free",
                                { "-t" },
                                R"(^Total:\s*(\d+).*$)",
                                { { "", TelemetryValueType::INTEGER } } } },
        { "locale",
          SystemTelemetryTuple{ "localectl",
                                {},
                                R"(\s*System Locale:\\s*LANG=\\s*(.*)$)",
                                { { "", TelemetryValueType::STRING } } } },
        { "uptime",
          SystemTelemetryTuple{ "head",
                                { "-100", "/proc/uptime" },
                                R"(^(\d+)\.\d*\s+.*$)",
                                { { "", TelemetryValueType::INTEGER } } } },
        { "timezone",
          SystemTelemetryTuple{ "date",
                                { "+%Z" },
                                R"(^([A-Z]{2,5})$)",
                                { { "", TelemetryValueType::STRING } } } },
        { "selinux",
            SystemTelemetryTuple{ "getenforce",
                                  {},
                                  R"(^(\w+)$)",
                                  { { "", TelemetryValueType::STRING } } } },
        { "apparmor",
                SystemTelemetryTuple{ "systemctl",
                                      {"is-enabled", "apparmor"},
                                      R"(^(\w+)$)",
                                      { { "", TelemetryValueType::STRING } } } },
        { "auditd",
                SystemTelemetryTuple{ "systemctl",
                                      {"is-enabled", "auditd"},
                                      R"(^(\w+)$)",
                                      { { "", TelemetryValueType::STRING } } } }
    };

    const SystemTelemetryConfig GL_systemTelemetryArraysConfig = {
        { "disks",
          SystemTelemetryTuple{
              "/bin/df",
              { "-T", "--local" },
              R"(^\s*\S+\s+(\S+)\s+\S+\s+\S+\s+(\d+)\s*.*$)",
              { { "fstype", TelemetryValueType::STRING }, { "free", TelemetryValueType::INTEGER } } } }
    };
} // namespace Telemetry
