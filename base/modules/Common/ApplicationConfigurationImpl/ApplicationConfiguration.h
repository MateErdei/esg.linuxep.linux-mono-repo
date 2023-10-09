// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <map>

namespace Common
{
    namespace ApplicationConfigurationImpl
    {
        constexpr char DefaultInstallLocation[] = "/opt/sophos-spl";

        class ApplicationConfiguration : public virtual Common::ApplicationConfiguration::IApplicationConfiguration
        {
        public:
            using configuration_data_t = std::map<std::string, std::string>;

            ApplicationConfiguration();
            std::string getData(const std::string& key) const override;

            void setData(const std::string& key, const std::string& data) override;
            void clearData(const std::string& key) override;

        private:
            configuration_data_t m_configurationData;
        };
    } // namespace ApplicationConfigurationImpl
} // namespace Common
