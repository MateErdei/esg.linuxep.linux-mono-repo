// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISdds3Wrapper.h"

namespace SulDownloader
{
    class Sdds3Wrapper : public ISdds3Wrapper
    {
    public:
        std::vector<sophlib::sdds3::Suite> getSuites(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& config) override;
        std::vector<sophlib::sdds3::PackageRef> getPackagesToInstall(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            sophlib::sdds3::Config& config,
            const sophlib::sdds3::Config& oldConfig) override;
        std::vector<sophlib::sdds3::PackageRef> getPackages(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& config) override;
        void extractPackagesTo(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& config,
            const std::string& path) override;
        void sync(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const std::string& url,
            sophlib::sdds3::Config& config,
            const sophlib::sdds3::Config& oldConfig) override;
        void saveConfig(sophlib::sdds3::Config& config, std::string& path) override;
        sophlib::sdds3::Config loadConfig(std::string& path) override;
        void Purge(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& new_config,
            const std::optional<sophlib::sdds3::Config>& old_config) override;
    };
    std::unique_ptr<ISdds3Wrapper>& sdds3WrapperStaticPointer();
} // namespace SulDownloader