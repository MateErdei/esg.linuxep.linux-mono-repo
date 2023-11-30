// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "sophlib/sdds3/Config.h"
#include "sophlib/sdds3/ConfigXml.h"
#include "sophlib/sdds3/PackageRef.h"
#include "sophlib/sdds3/SyncLogic.h"

namespace SulDownloader
{
    class ISdds3Wrapper
    {
    public:
        virtual ~ISdds3Wrapper() = default;
        virtual std::vector<sophlib::sdds3::Suite> getSuites(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& config) = 0;
        virtual std::vector<sophlib::sdds3::PackageRef> getPackagesToInstall(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            sophlib::sdds3::Config& config,
            const sophlib::sdds3::Config& oldConfig) = 0;
        virtual std::vector<sophlib::sdds3::PackageRef> getPackages(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& config) = 0;
        virtual void extractPackagesTo(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& config,
            const std::string& path) = 0;
        virtual void sync(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const std::string& url,
            sophlib::sdds3::Config& config,
            const sophlib::sdds3::Config& oldConfig) = 0;
        virtual void saveConfig(sophlib::sdds3::Config& config, std::string& path) = 0;
        virtual sophlib::sdds3::Config loadConfig(std::string& path) = 0;
        virtual void Purge(
            sophlib::sdds3::Session& session,
            const sophlib::sdds3::Repo& repo,
            const sophlib::sdds3::Config& new_config,
            const std::optional<sophlib::sdds3::Config>& old_config) = 0;
    };
    /**
     * Return a BORROWED pointer to a static ISdds3Wrapper instance.
     *
     * Do not delete this yourself.
     *
     * @return BORROWED ISdds3Wrapper pointer
     */
    ISdds3Wrapper* sdds3Wrapper();
} // namespace SulDownloader