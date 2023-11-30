// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "SulDownloader/sdds3/ISdds3Wrapper.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace SulDownloader;

class MockSdds3Wrapper : public SulDownloader::ISdds3Wrapper
{
public:
    MOCK_METHOD(std::vector<sophlib::sdds3::Suite>, getSuites, (sophlib::sdds3::Session& session, const sophlib::sdds3::Repo& repo, const sophlib::sdds3::Config& config));
    MOCK_METHOD(std::vector<sophlib::sdds3::PackageRef>, getPackagesToInstall, (sophlib::sdds3::Session& session, const sophlib::sdds3::Repo& repo, sophlib::sdds3::Config& config, const sophlib::sdds3::Config& oldConfig));
    MOCK_METHOD(std::vector<sophlib::sdds3::PackageRef>, getPackages, (sophlib::sdds3::Session& session, const sophlib::sdds3::Repo& repo, const sophlib::sdds3::Config& config));
    MOCK_METHOD(void, extractPackagesTo, (sophlib::sdds3::Session& session, const sophlib::sdds3::Repo& repo, const sophlib::sdds3::Config& config, const std::string& path));
    MOCK_METHOD(void, sync, (sophlib::sdds3::Session& session, const sophlib::sdds3::Repo& repo, const std::string& url, sophlib::sdds3::Config& config, const sophlib::sdds3::Config& oldConfig));
    MOCK_METHOD(void, saveConfig, (sophlib::sdds3::Config& config, std::string& path));
    MOCK_METHOD(sophlib::sdds3::Config, loadConfig, (std::string& path));
    MOCK_METHOD(void, Purge, (sophlib::sdds3::Session& session, const sophlib::sdds3::Repo& repo, const sophlib::sdds3::Config& new_config, const std::optional<sophlib::sdds3::Config>& old_config), (override));
};
