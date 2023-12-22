// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/ConfigFile/ConfigFile.h"

#include "Common/OSUtilities/IPlatformUtils.h"

#include <map>

namespace thininstaller::telemetry
{
    class JsonBuilder
    {
    public:
        using ConfigFile = Common::ConfigFile::ConfigFile;
        using map_t = std::map<std::string, ConfigFile>;
        JsonBuilder(const ConfigFile& settings, const map_t& results);
        std::string build(const Common::OSUtilities::IPlatformUtils& platform);
        std::string timestamp();
        std::string tenantId();
        std::string machineId();
        static std::string generateTimeStamp();
    private:
        const Common::ConfigFile::ConfigFile& settings_;
        const map_t& results_;
        std::string timestamp_;
        std::string tenantId_;
        std::string machineId_;
    };
}

