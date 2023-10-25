// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "SystemTelemetryConfig.h"
#include "SystemTelemetryConsts.h"

namespace Telemetry
{
    SystemTelemetryConfig systemTelemetryObjectsConfig()
    {
        return SystemTelemetryConfig{
            { "kernel",
              SystemTelemetryTuple{
                  "hostnamectl", {}, R"(^\s*Kernel:\s*(.*)$)", { { "", TelemetryValueType::STRING } } } },
            { "cpu-cores",
              SystemTelemetryTuple{ "lscpu", {}, R"(^CPU\(s\):\s*(\d+)$)", { { "", TelemetryValueType::INTEGER } } } },
            { "memory-total",
              SystemTelemetryTuple{
                  "free", { "-t" }, R"(^Total:\s*(\d+).*$)", { { "", TelemetryValueType::INTEGER } } } },
            { "locale",
              SystemTelemetryTuple{
                  "localectl", {}, R"(\s*System Locale:\s*LANG=\s*(.*)$)", { { "", TelemetryValueType::STRING } } } },
            { "uptime",
              SystemTelemetryTuple{ "head",
                                    { "-100", "/proc/uptime" },
                                    R"(^(\d+)\.\d*\s+.*$)",
                                    { { "", TelemetryValueType::INTEGER } } } },
            { "timezone",
              SystemTelemetryTuple{ "date", { "+%Z" }, R"(^([A-Z]{2,5})$)", { { "", TelemetryValueType::STRING } } } },
            { "selinux",
              SystemTelemetryTuple{ "getenforce", {}, R"(^(\w+)$)", { { "", TelemetryValueType::STRING } } } },
            { "apparmor",
              SystemTelemetryTuple{
                  "systemctl", { "is-active", "apparmor" }, R"(^(\w+)$)", { { "", TelemetryValueType::STRING } } } },
            { "auditd",
              SystemTelemetryTuple{
                  "systemctl", { "is-active", "auditd" }, R"(^(\w+)$)", { { "", TelemetryValueType::STRING } } } },
            { "architecture",
                    SystemTelemetryTuple{
                            "uname", { "-m"}, R"(^(\w+)$)", { { "", TelemetryValueType::STRING } } } }
        };
    }

    SystemTelemetryConfig systemTelemetryArraysConfig()
    {
        return SystemTelemetryConfig{ { "disks",
                                        SystemTelemetryTuple{ "/bin/df",
                                                              { "-T", "--local" },
                                                              R"(^\s*\S+\s+(\S+)\s+\S+\s+\S+\s+(\d+)\s*.*$)",
                                                              { { "fstype", TelemetryValueType::STRING },
                                                                { "free", TelemetryValueType::INTEGER } } } } };
    }
} // namespace Telemetry
