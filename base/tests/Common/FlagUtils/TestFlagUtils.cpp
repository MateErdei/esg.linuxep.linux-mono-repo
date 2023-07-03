/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/FlagUtils/FlagUtils.h"

#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

class FlagUtils: public LogOffInitializedTests{};

TEST_F(FlagUtils, testisFlagSetReturnsTruewhenFlagIsEnabled)
{
    std::string content = R"({"sdds3.force-update" : true})";
    ASSERT_EQ(true, Common::FlagUtils::isFlagSet("sdds3.force-update", content));
}

TEST_F(FlagUtils, testisFlagSetReturnsFalseWhenFlagIsNotEnabled)
{
    std::string falseFlagContent = R"({"sdds3.force-update" : false})";
    std::string diffFlagContent = R"({"xdr.enabled" : true})";

    ASSERT_EQ(false, Common::FlagUtils::isFlagSet("sdds3.force-update", falseFlagContent));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet("sdds3.force-update", diffFlagContent));
}

TEST_F(FlagUtils, testisFlagSetReturnsFalseWhenInvalidJson)
{
    std::string jsonNoQuotes = R"({sdds3.force-update : true})";
    std::string jsonNoBool = R"({"sdds3.force-update" : "true"})";

    ASSERT_EQ(false, Common::FlagUtils::isFlagSet("sdds3.force-update", "{}"));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet("sdds3.force-update", "}"));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet("sdds3.force-update", jsonNoQuotes));
    ASSERT_EQ(false, Common::FlagUtils::isFlagSet("sdds3.force-update", jsonNoBool));
}