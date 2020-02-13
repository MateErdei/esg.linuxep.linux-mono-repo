/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/NamedScanRunner.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

using namespace avscanner::avscannerimpl;

TEST(NamedScanRunner, TestNamedScanConfigDeserialisation) // NOLINT
{
    std::string expectedScanName = "testScan";
    std::vector<std::string> expectedExclusions;
    expectedExclusions.emplace_back("/exclude1.txt");
    expectedExclusions.emplace_back("/exclude2/");
    expectedExclusions.emplace_back("/exclude3/*/*.txt");
    bool scanHardDisc = true;
    bool scanNetwork = false;
    bool scanOptical = true;
    bool scanRemovable = false;

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder scanConfigIn = message.initRoot<Sophos::ssplav::NamedScan>();
    scanConfigIn.setName(expectedScanName);
    auto exclusions = scanConfigIn.initExcludePaths(expectedExclusions.size());
    for (unsigned i=0; i < expectedExclusions.size(); i++)
    {
        exclusions.set(i, expectedExclusions[i]);
    }
    scanConfigIn.setScanHardDisc(scanHardDisc);
    scanConfigIn.setScanNetwork(scanNetwork);
    scanConfigIn.setScanOptical(scanOptical);
    scanConfigIn.setScanRemovable(scanRemovable);

    Sophos::ssplav::NamedScan::Reader scanConfigOut = message.getRoot<Sophos::ssplav::NamedScan>();

    NamedScanRunner runner(scanConfigOut);

    NamedScanConfig config = runner.getConfig();
    EXPECT_EQ(config.m_scanName, expectedScanName);
    EXPECT_EQ(config.m_excludePaths, expectedExclusions);
    EXPECT_EQ(config.m_scanHardDisc, scanHardDisc);
    EXPECT_EQ(config.m_scanNetwork, scanNetwork);
    EXPECT_EQ(config.m_scanOptical, scanOptical);
    EXPECT_EQ(config.m_scanRemovable, scanRemovable);
}
