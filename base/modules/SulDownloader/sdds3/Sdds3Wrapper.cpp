// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Sdds3Wrapper.h"

#include <filesystem>

class path;
namespace SulDownloader
{
    std::vector<sophlib::sdds3::Suite> Sdds3Wrapper::getSuites(
        sophlib::sdds3::Session& session,
        const sophlib::sdds3::Repo& repo,
        const sophlib::sdds3::Config& config)
    {
        return sophlib::sdds3::GetSuites(session, repo, config);
    }

    std::vector<sophlib::sdds3::PackageRef> Sdds3Wrapper::getPackagesToInstall(
        sophlib::sdds3::Session& session,
        const sophlib::sdds3::Repo& repo,
        sophlib::sdds3::Config& config,
        const sophlib::sdds3::Config& oldConfig)
    {
        return sophlib::sdds3::GetPackagesToInstall(session, repo, config, oldConfig);
    }

    std::vector<sophlib::sdds3::PackageRef> Sdds3Wrapper::getPackages(
        sophlib::sdds3::Session& session,
        const sophlib::sdds3::Repo& repo,
        const sophlib::sdds3::Config& config)
    {
        return sophlib::sdds3::GetPackages(session, repo, config);
    }

    void Sdds3Wrapper::extractPackagesTo(
        sophlib::sdds3::Session& session,
        const sophlib::sdds3::Repo& repo,
        const sophlib::sdds3::Config& config,
        const std::string& path)
    {
        sophlib::sdds3::ExtractTo(session, repo, config, path);
    }
    void Sdds3Wrapper::sync(
        sophlib::sdds3::Session& session,
        const sophlib::sdds3::Repo& repo,
        const std::string& url,
        sophlib::sdds3::Config& config,
        const sophlib::sdds3::Config& oldConfig)
    {
        sophlib::sdds3::Sync(session, repo, url, config, oldConfig);
    }
    void Sdds3Wrapper::saveConfig(sophlib::sdds3::Config& config, std::string& path)
    {
        std::filesystem::path configFilePath(path);
        sophlib::sdds3::ConfigXml::Save(config, configFilePath);
    }
    sophlib::sdds3::Config Sdds3Wrapper::loadConfig(std::string& path)
    {
        std::filesystem::path configFilePath(path);
        return sophlib::sdds3::ConfigXml::Load(configFilePath);
    }

    void Sdds3Wrapper::Purge(
        sophlib::sdds3::Session& session,
        const sophlib::sdds3::Repo& repo,
        const sophlib::sdds3::Config& new_config,
        const std::optional<sophlib::sdds3::Config>& old_config)
    {
        sophlib::sdds3::Purge(session, repo, new_config, old_config);
    }

    std::unique_ptr<ISdds3Wrapper>& sdds3WrapperStaticPointer()
    {
        static std::unique_ptr<ISdds3Wrapper> instance = std::unique_ptr<ISdds3Wrapper>(new Sdds3Wrapper());
        return instance;
    }
} // namespace SulDownloader
SulDownloader::ISdds3Wrapper* SulDownloader::sdds3Wrapper()
{
    return SulDownloader::sdds3WrapperStaticPointer().get();
}
