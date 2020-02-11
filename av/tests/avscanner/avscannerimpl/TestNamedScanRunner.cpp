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

    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Builder scanConfigIn = message.initRoot<Sophos::ssplav::NamedScan>();
    scanConfigIn.setName(expectedScanName);
    Sophos::ssplav::NamedScan::Reader scanConfigOut = message.getRoot<Sophos::ssplav::NamedScan>();

    std::unique_ptr<NamedScanRunner> runner = std::make_unique<NamedScanRunner>(scanConfigOut);

    EXPECT_EQ(runner->getScanName(), expectedScanName);

}


