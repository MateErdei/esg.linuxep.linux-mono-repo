// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include <json/json.h>

namespace
{
    using EnumStringMap = const std::map<int, const std::string>;

    EnumStringMap THREAT_SOURCE_MAP = { { 0, "ML" },
                                        { 1, "SAV" },
                                        { 2, "Rep" },
                                        { 3, "Blocklist" },
                                        { 4, "AppWL" },
                                        { 5, "AMSI" },
                                        { 6, "IPS" },
                                        { 7, "Behavioral" },
                                        { 8, "VDL" },
                                        { 9, "HBT" },
                                        { 10, "MTD" },
                                        { 11, "Web" },
                                        { 12, "Device Control" } };

    EnumStringMap THREAT_TYPE_MAP = { { 1, "Malware" },         { 2, "Pua" },   { 4, "Suspicious Behaviour" },   { 7, "Blocked" },
                                      { 8, "Amsi Protection" }, { 100, "App" }, { 102, "Data Loss Prevention" },
                                      { 103, "Device Control" } };

    EnumStringMap THREAT_ITEM_TYPE_MAP = { { 1, "FILE" },
                                        { 2, "PROCESS" },
                                        { 3, "URL" },
                                        { 4, "NETWORK" },
                                        { 5, "REGISTRY" },
                                        { 6, "THREAD" },
                                        { 7, "DEVICE" } };
} // namespace