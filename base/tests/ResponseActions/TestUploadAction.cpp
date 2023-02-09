// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/UploadFileAction.h"
#include "modules/Common/UtilityImpl/TimeUtils.h"

#include "tests/Common/Helpers/MemoryAppender.h"

#include <json.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class UploadFileTests : public MemoryAppenderUsingTests
{
public:
    UploadFileTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
    nlohmann::json getDefaultUploadObject()
    {
        nlohmann::json action;
        action["targetFile"] = "path";
        action["url"] = "https://s3.com/somewhere";
        action["compress"] = false;
        action["password"] = "";
        action["expiration"] = 1444444;
        action["timeout"] = 10;
        action["maxUploadSizeBytes"] = 10000000;
        return action;
    }
};

TEST_F(UploadFileTests, cannotParseActions)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction;

    std::string response = uploadFileAction.run("");
    nlohmann::json reponseJson = nlohmann::json::parse(response);
    EXPECT_EQ(reponseJson["result"],1);
    EXPECT_EQ(reponseJson["errorType"],"invalid_path");
    EXPECT_EQ(reponseJson["errorMessage"],"Error parsing command from Central");

}
