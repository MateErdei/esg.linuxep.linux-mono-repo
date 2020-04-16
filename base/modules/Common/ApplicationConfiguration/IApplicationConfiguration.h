/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>
#include <string>
namespace Common
{
    namespace ApplicationConfiguration
    {
        static const std::string SOPHOS_INSTALL = "SOPHOS_INSTALL"; // NOLINT
        static const std::string TELEMETRY_RESTORE_DIR = "RESTORE_DIR"; // NOLINT

        class IApplicationConfiguration
        {
        public:
            virtual ~IApplicationConfiguration() = default;
            virtual std::string getData(const std::string& key) const = 0;
            virtual void setData(const std::string& key, const std::string& data) = 0;
        };

        IApplicationConfiguration& applicationConfiguration();
    } // namespace ApplicationConfiguration
} // namespace Common
