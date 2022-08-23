/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "ISdds3Wrapper.h"

namespace SulDownloader
{
    class Sdds3Wrapper : public ISdds3Wrapper
    {
    public:
        std::vector<sdds3::Suite> getSuites(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config) override;
        std::vector<sdds3::PackageRef> getPackagesIncludingSupplements(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config) override;
        std::vector<sdds3::PackageRef> getPackagesToInstall(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            sdds3::Config& config,
            const sdds3::Config& oldConfig) override;
        std::vector<sdds3::PackageRef> getPackages(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config) override;
        void extractPackagesTo(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config,
            const std::string& path) override;
        void sync(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const std::string& url,
            sdds3::Config& config,
            const sdds3::Config& oldConfig) override;
        void saveConfig(sdds3::Config& config, std::string& path) override;
        sdds3::Config loadConfig(std::string& path) override;
    };
    std::unique_ptr<ISdds3Wrapper>& sdds3WrapperStaticPointer();
}