/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <Config.h>
#include <PackageRef.h>
#include <SyncLogic.h>


namespace SulDownloader
{
    class ISdds3Wrapper
    {
    public:
        virtual ~ISdds3Wrapper() = default;
        virtual std::vector<sdds3::Suite> getSuites(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config) = 0;
        virtual std::vector<sdds3::PackageRef> getPackagesIncludingSupplements(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config) = 0;
        virtual std::vector<sdds3::PackageRef> getPackagesToInstall(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            sdds3::Config& config,
            const sdds3::Config& oldConfig) = 0;
        virtual std::vector<sdds3::PackageRef> getPackages(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config) = 0;
        virtual void extractPackagesTo(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const sdds3::Config& config,
            const std::string& path) = 0;
        virtual void sync(
            sdds3::Session& session,
            const sdds3::Repo& repo,
            const std::string& url,
            sdds3::Config& config,
            const sdds3::Config& oldConfig) = 0;
        virtual void saveConfig(sdds3::Config& config, std::string& path) = 0;
        virtual sdds3::Config loadConfig(std::string& path) = 0;
    };
    /**
    * Return a BORROWED pointer to a static ISdds3Wrapper instance.
    *
    * Do not delete this yourself.
    *
    * @return BORROWED ISdds3Wrapper pointer
    */
    ISdds3Wrapper* sdds3Wrapper();
}