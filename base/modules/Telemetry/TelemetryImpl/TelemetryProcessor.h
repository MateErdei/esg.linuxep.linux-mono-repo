/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITelemetryProvider.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <utility>
using namespace Common::Telemetry;

namespace Telemetry
{
    class TelemetryProcessor
    {
    public:
        TelemetryProcessor(std::vector<std::shared_ptr<ITelemetryProvider>> telemetryProviders, size_t maxJsonBytes);
        void gatherTelemetry();
        void saveAndSendTelemetry();
        std::string getSerialisedTelemetry();

    private:
        Common::Telemetry::TelemetryHelper m_telemetryHelper;
        std::vector<std::shared_ptr<ITelemetryProvider>> m_telemetryProviders;
        size_t m_maxJsonSizeBytes;

        void addTelemetry(const std::string& sourceName, const std::string& json);
    };

}