/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <modules/CommsComponent/CommsMsg.h>
#include <modules/CommsComponent/CommsConfig.h>


using namespace CommsComponent;

::testing::AssertionResult requestAreEquivalent(
        std::stringstream & s,
        const Common::HttpSender::RequestConfig& expected,
        const Common::HttpSender::RequestConfig& actual);

::testing::AssertionResult responseAreEquivalent(
        std::stringstream & s,
        const Common::HttpSender::HttpResponse& expected,
        const Common::HttpSender::HttpResponse& actual);

::testing::AssertionResult configAreEquivalent(
        std::stringstream & s,
        const CommsComponent::CommsConfig& expected,
        const CommsComponent::CommsConfig& actual);

::testing::AssertionResult commsMsgAreEquivalent(
        const char* m_expr,
        const char* n_expr,
        const CommsComponent::CommsMsg& expected,
        const CommsComponent::CommsMsg& actual);