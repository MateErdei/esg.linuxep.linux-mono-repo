/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/PluginProtocol/DataMessage.h"
#include "gmock/gmock.h"
#include "gtest/gtest-printers.h"
#include "gtest/gtest.h"

class TestCompare : public ::testing::Test
{
public:
    ::testing::AssertionResult dataMessageSimilar(
        const char* m_expr,
        const char* n_expr,
        const Common::PluginProtocol::DataMessage& expected,
        const Common::PluginProtocol::DataMessage& resulted);
};
