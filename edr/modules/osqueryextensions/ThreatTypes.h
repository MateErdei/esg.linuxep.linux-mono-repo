/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
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

    EnumStringMap THREAT_TYPE_MAP = { { 1, "Malware" },         { 2, "Pua" },   { 7, "Blocked" },
                                      { 8, "Amsi Protection" }, { 100, "App" }, { 102, "Data Loss Prevention" },
                                      { 103, "Device Control" } };

    enum ItemType : unsigned char
    {
        FILE = 1,
        PROCESS = 2,
        URL = 3,
        NETWORK = 4,
        REGISTRY = 5,
        THREAD = 6,
        DEVICE = 7,
    };
} // namespace