/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_TESTCOMPARE_H
#define EVEREST_BASE_TESTCOMPARE_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "gtest/gtest-printers.h"

#include "Common/PluginProtocol/DataMessage.h"


class TestCompare : public ::testing::Test
{
public:
    ::testing::AssertionResult dataMessageSimilar(const char *m_expr,
                                                  const char *n_expr,
                                                  const Common::PluginProtocol::DataMessage &expected,
                                                  const Common::PluginProtocol::DataMessage &resulted);
};


#endif //EVEREST_BASE_TESTCOMPARE_H
