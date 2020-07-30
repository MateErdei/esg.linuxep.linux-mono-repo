/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

*****************************************************************************************************/

#include "LogInitializedTests.h"

void test_common::initialize_logging()
{
    static Common::Logging::ConsoleLoggingSetup m_loggingSetup;
}
