/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <UpdateSchedulerImpl/UpdateSchedulerUtils.h>
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineData.h>

#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <gtest/gtest.h>
#include <json.hpp>

class UpdateSchedulerUtils: public LogOffInitializedTests{};

TEST_F(UpdateSchedulerUtils, allHealthGoodWhenEndpointStateGood)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setInstallState("0");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 0;
    expectedHealth["installState"] = 0;
    expectedHealth["overall"] = 0;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}

TEST_F(UpdateSchedulerUtils, OverallHealthBadWhenInstallStateIsBad)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("0");
    expectedStateMachineData.setInstallState("1");
    expectedStateMachineData.setDownloadFailedSinceTime("10");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 0;
    expectedHealth["installState"] = 1;
    expectedHealth["overall"] = 1;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}

TEST_F(UpdateSchedulerUtils, OverallHealthBadWhenDownloadStateIsBad)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("1");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("10");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 1;
    expectedHealth["installState"] = 0;
    expectedHealth["overall"] = 1;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}

TEST_F(UpdateSchedulerUtils, allHealthBadWhendownloadStateeIsGarbage)
{
    UpdateSchedulerImpl::StateData::StateMachineData expectedStateMachineData;
    expectedStateMachineData.setDownloadState("788");
    expectedStateMachineData.setInstallState("0");
    expectedStateMachineData.setDownloadFailedSinceTime("10");

    nlohmann::json actualHealth = nlohmann::json::parse(UpdateSchedulerImpl::UpdateSchedulerUtils::calculateHealth(expectedStateMachineData));
    nlohmann::json expectedHealth;
    expectedHealth["downloadState"] = 1;
    expectedHealth["installState"] = 0;
    expectedHealth["overall"] = 1;
    ASSERT_EQ(expectedHealth.dump(),actualHealth.dump());
}