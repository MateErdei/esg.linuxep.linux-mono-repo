// Copyright 2022 Sophos Limited. All rights reserved.

#include "scan_messages/RestoreReport.h"

#include <gtest/gtest.h>

using namespace scan_messages;
using namespace ::testing;

TEST(TestRestoreReport, validate)
{
    EXPECT_NO_THROW(({
        const RestoreReport restoreReport{ 0, "/path/eicar.txt", "00000000-0000-0000-0000-000000000000", true };
        restoreReport.validate();
    }));

    EXPECT_ANY_THROW(({
        const RestoreReport restoreReport{ 0, std::string{ '\0' }, "00000000-0000-0000-0000-000000000000", true };
        restoreReport.validate();
    }));

    EXPECT_ANY_THROW(({
        const RestoreReport restoreReport{ 0, "/path/eicar.txt", "invalid UUID", true };
        restoreReport.validate();
    }));
}

TEST(TestRestoreReport, serialisationDeserialisation)
{
    const RestoreReport restoreReport{ 0, "/path/eicar.txt", "00000000-0000-0000-0000-000000000000", true };
    auto builder = restoreReport.serialise();

    auto reader = builder->getRoot<Sophos::ssplav::RestoreReport>();

    const RestoreReport deserialisedRestoreReport(reader);

    EXPECT_EQ(deserialisedRestoreReport, restoreReport);
}