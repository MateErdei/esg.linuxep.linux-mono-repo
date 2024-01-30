// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "ALCPoliciesExample.h"

#include "pluginimpl/OsqueryConfigurator.h"
#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include <gtest/gtest.h>


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


TEST_F(TestOsqueryConfigurator, enableAuditDataCollectionInternalReturnsExpectedValueGivenSetOfInputs)
{
    bool disableAuditDInPluginConfig = false;
    TestableOsqueryConfigurator osqueryConfigurator(disableAuditDInPluginConfig);
    ASSERT_FALSE(osqueryConfigurator.shouldAuditDataCollectionBeEnabled());

    disableAuditDInPluginConfig = true;
    osqueryConfigurator = TestableOsqueryConfigurator(disableAuditDInPluginConfig);
    ASSERT_TRUE(osqueryConfigurator.shouldAuditDataCollectionBeEnabled());
}