/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gmock/gmock.h"

#include <SulDownloader/sdds3/ISdds3Wrapper.h>

using namespace ::testing;
using namespace SulDownloader;

class MockSdds3Wrapper : public SulDownloader::ISdds3Wrapper
{
public:
    MOCK_METHOD3(getPackagesIncludingSupplements, std::vector<sdds3::PackageRef> (sdds3::Session& session, const sdds3::Repo& repo, const sdds3::Config& config));
    MOCK_METHOD4(getPackagesToInstall, std::vector<sdds3::PackageRef> (sdds3::Session& session, const sdds3::Repo& repo, sdds3::Config& config, const sdds3::Config& oldConfig));
    MOCK_METHOD3(getPackages, std::vector<sdds3::PackageRef> (sdds3::Session& session, const sdds3::Repo& repo, const sdds3::Config& config));
    MOCK_METHOD4(extractPackagesTo, void (sdds3::Session& session, const sdds3::Repo& repo, const sdds3::Config& config, const std::string& path));
    MOCK_METHOD5(sync, void(sdds3::Session& session, const sdds3::Repo& repo, const std::string& url, sdds3::Config& config, const sdds3::Config& oldConfig));
    MOCK_METHOD2(saveConfig, void(sdds3::Config& config, std::string& path));
    MOCK_METHOD1(loadConfig, sdds3::Config (std::string& path));
};
