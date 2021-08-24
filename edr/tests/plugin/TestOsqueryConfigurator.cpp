/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <modules/pluginimpl/OsqueryConfigurator.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <tests/osqueryclient/MockOsqueryClient.h>

#include <gtest/gtest.h>
#include <modules/pluginimpl/PluginUtils.h>

using namespace Plugin;
class TestableOsqueryConfigurator : public OsqueryConfigurator
{

public:
    explicit TestableOsqueryConfigurator(
        bool disableSystemAuditDAndTakeOwnershipOfNetlink) : OsqueryConfigurator()
    {
        m_disableAuditDInPluginConfig = disableSystemAuditDAndTakeOwnershipOfNetlink;
    }
};

class TestOsqueryConfigurator : public LogOffInitializedTests{};


TEST_F(TestOsqueryConfigurator, enableAuditDataCollectionInternalReturnsExpectedValueGivenSetOfInputs) // NOLINT
{
    bool disableAuditDInPluginConfig = false;
    TestableOsqueryConfigurator osqueryConfigurator(disableAuditDInPluginConfig);
    ASSERT_FALSE(osqueryConfigurator.shouldAuditDataCollectionBeEnabled());

    disableAuditDInPluginConfig = true;
    osqueryConfigurator = TestableOsqueryConfigurator(disableAuditDInPluginConfig);
    ASSERT_TRUE(osqueryConfigurator.shouldAuditDataCollectionBeEnabled());
}