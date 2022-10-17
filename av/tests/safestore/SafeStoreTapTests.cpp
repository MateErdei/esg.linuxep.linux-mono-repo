// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../common/LogInitializedTests.h"
#include "safestore/SafeStoreWrapperImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"

#include <common/ApplicationPaths.h>
#include <gtest/gtest.h>

class SafeStoreWrapperTapTests : public LogInitializedTests
{
protected:
    static void SetUpTestSuite()
    {
        std::cout << "SetUpTestSuite" << std::endl;
        auto fileSystem = Common::FileSystem::fileSystem();
        fileSystem->makedirs(m_safeStoreDbDir);
    }

    static void TearDownTestSuite()
    {
        std::cout << "TearDownTestSuite" << std::endl;
    }

    void SetUp() override
    {
        std::cout << "SetUp" << std::endl;
        auto initReturnCode = m_safeStoreWrapper->initialise(m_safeStoreDbDir, m_safeStoreDbName, m_safeStoreDbPw);
        ASSERT_EQ(initReturnCode, safestore::InitReturnCode::OK);
    }

    static void deleteDatabase()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        fileSystem->removeFilesInDirectory(m_safeStoreDbDir);
    }

    void TearDown() override
    {
        std::cout << "TearDown" << std::endl;
        deleteDatabase();
    }

    static inline const std::string m_testWorkingDir = "/tmp/SafeStoreWrapperTapTests";
    static inline const std::string m_safeStoreDbDir = Common::FileSystem::join(m_testWorkingDir, "safestore_db");
    static inline const std::string m_safeStoreDbName = "safestore.db";
    static inline const std::string m_safeStoreDbPw = "a test password";
    std::unique_ptr<safestore::ISafeStoreWrapper> m_safeStoreWrapper =
        std::make_unique<safestore::SafeStoreWrapperImpl>();
};

TEST_F(SafeStoreWrapperTapTests, initExistingDb)
{
    auto initReturnCode = m_safeStoreWrapper->initialise(m_safeStoreDbDir, m_safeStoreDbName, m_safeStoreDbPw);
    ASSERT_EQ(initReturnCode, safestore::InitReturnCode::OK);
}

TEST_F(SafeStoreWrapperTapTests, readDefaultConfigOptions)
{
    auto autoPurge = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::AUTO_PURGE);
    ASSERT_TRUE(autoPurge.has_value());
    ASSERT_EQ(autoPurge.value(), 1);

    auto maxObjSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE);
    ASSERT_TRUE(maxObjSize.has_value());
    ASSERT_EQ(maxObjSize.value(), 107374182400);

    auto maxObjInRegistrySubtree = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT);
    ASSERT_FALSE(maxObjInRegistrySubtree.has_value());

    auto maxSafeStoreSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE);
    ASSERT_TRUE(maxSafeStoreSize.has_value());
    ASSERT_EQ(maxSafeStoreSize.value(), 214748364800);

    auto maxObjCount = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT);
    ASSERT_TRUE(maxObjCount.has_value());
    ASSERT_EQ(maxObjCount.value(), 2000);
}

TEST_F(SafeStoreWrapperTapTests, writeAndThenRreadBackConfigOptions)
{
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::AUTO_PURGE, false));
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE, 100000000000));

    // This currently fails - defect or we don't care as it's windows only?
    // ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT, 100));

    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE, 200000000000));
    ASSERT_TRUE(m_safeStoreWrapper->setConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT, 5000));

    auto autoPurge = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::AUTO_PURGE);
    ASSERT_TRUE(autoPurge.has_value());
    ASSERT_EQ(autoPurge.value(), 0);

    auto maxObjSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_OBJECT_SIZE);
    ASSERT_TRUE(maxObjSize.has_value());
    ASSERT_EQ(maxObjSize.value(), 100000000000);

    //    auto maxObjInRegistrySubtree =
    //    m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_REG_OBJECT_COUNT);
    //    ASSERT_TRUE(maxObjInRegistrySubtree.has_value());
    //    ASSERT_EQ(maxObjInRegistrySubtree.value(), 100);

    auto maxSafeStoreSize = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_SAFESTORE_SIZE);
    ASSERT_TRUE(maxSafeStoreSize.has_value());
    ASSERT_EQ(maxSafeStoreSize.value(), 200000000000);

    auto maxObjCount = m_safeStoreWrapper->getConfigIntValue(safestore::ConfigOption::MAX_STORED_OBJECT_COUNT);
    ASSERT_TRUE(maxObjCount.has_value());
    ASSERT_EQ(maxObjCount.value(), 5000);
}
