/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "Sdds3Wrapper.h"
#include <filesystem>

class path;
std::vector<sdds3::PackageRef> Sdds3Wrapper::getPackagesIncludingSupplements(sdds3::Session& session, const sdds3::Repo& repo, const sdds3::Config& config)
{
    return sdds3::get_packages_including_supplements(session, repo, config);
}
std::vector<sdds3::PackageRef> Sdds3Wrapper::getPackagesToInstall(sdds3::Session& session, const sdds3::Repo& repo, sdds3::Config& config, const sdds3::Config& oldConfig)
{
    return sdds3::get_packages_to_install(session, repo, config, oldConfig);
}

std::vector<sdds3::PackageRef> Sdds3Wrapper::getPackages(sdds3::Session& session, const sdds3::Repo& repo, const sdds3::Config& config)
{
    return sdds3::get_packages(session, repo, config);
}

void Sdds3Wrapper::extractPackagesTo(sdds3::Session& session, const sdds3::Repo& repo, const sdds3::Config& config, const std::string& path)
{
    sdds3::extract_to(session, repo, config, path);
}
void Sdds3Wrapper::sync(sdds3::Session& session, const sdds3::Repo& repo, const std::string& url, sdds3::Config& config, const sdds3::Config& oldConfig)
{
    sdds3::sync(session, repo, url, config, oldConfig);
}
void Sdds3Wrapper::saveConfig(sdds3::Config& config, std::string& path)
{
    std::filesystem::path configFilePath(path);
    sdds3::save_config(config, configFilePath);

}
sdds3::Config Sdds3Wrapper::loadConfig(std::string& path)
{
    std::filesystem::path configFilePath(path);
    return sdds3::load_config(configFilePath);
}
