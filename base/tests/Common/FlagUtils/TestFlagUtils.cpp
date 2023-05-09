/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FlagUtils/FlagUtils.h>
#include <UpdateSchedulerImpl/UpdateSchedulerUtils.h>

#include <tests/Common/Helpers/LogInitializedTests.h>

#include <gtest/gtest.h>

class FlagUtils: public LogOffInitializedTests{};

TEST_F(FlagUtils, testisFlagSetReturnsTruewhenFlagIsEnabled)
{
    std::string content = R"({"sdds3.force-update" : true})";
    ASSERT_EQ(true, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, content));
}

TEST_F(FlagUtils, testisFlagSetReturnsFalseWhenFlagIsNotEnabled)
{
    std::string falseFlagContent = R"({"sdds3.force-update" : false})";
    std::string diffFlagContent = R"({"xdr.enabled" : true})";

    ASSERT_EQ(false, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, falseFlagContent));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, diffFlagContent));
}

TEST_F(FlagUtils, testisFlagSetReturnsFalseWhenInvalidJson)
{
    std::string jsonNoQuotes = R"({sdds3.force-update : true})";
    std::string jsonNoBool = R"({"sdds3.force-update" : "true"})";

    ASSERT_EQ(false, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, "{}"));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, "}"));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, jsonNoQuotes));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet(UpdateSchedulerImpl::UpdateSchedulerUtils::FORCE_UPDATE_ENABLED_FLAG, jsonNoBool));
}