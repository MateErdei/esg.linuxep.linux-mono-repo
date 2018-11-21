/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "TestCompare.h"


::testing::AssertionResult TestCompare::dataMessageSimilar(const char *m_expr,
                                              const char *n_expr,
                                              const Common::PluginProtocol::DataMessage &expected,
                                              const Common::PluginProtocol::DataMessage &resulted)
{
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";

    if (expected.m_applicationId != resulted.m_applicationId)
    {
        return ::testing::AssertionFailure() << s.str() << " Application Id differ: \n expected: "
                                             << expected.m_applicationId
                                             << "\n result: " << resulted.m_applicationId;
    }

    if (expected.m_pluginName != resulted.m_pluginName)
    {
        return ::testing::AssertionFailure() << s.str() << " PluginName differ: \n expected: "
                                             << expected.m_pluginName
                                             << "\n result: " << resulted.m_pluginName;
    }

    if (expected.m_command != resulted.m_command)
    {
        return ::testing::AssertionFailure() << s.str() << " command differ: \n expected: "
                                             << Common::PluginProtocol::ConvertCommandEnumToString(expected.m_command)
                                             << "\n result: "
                                             << Common::PluginProtocol::ConvertCommandEnumToString(resulted.m_command);
    }

    if (expected.m_error != resulted.m_error)
    {
        return ::testing::AssertionFailure() << s.str() << " Error message differ: \n expected: "
                                             << expected.m_error
                                             << "\n result: " << resulted.m_error;
    }

    if (expected.m_acknowledge != resulted.m_acknowledge)
    {
        return ::testing::AssertionFailure() << s.str() << " Acknowledge differ: \n expected: "
                                             << ::testing::PrintToString(expected.m_acknowledge)
                                             << "\n result: " << ::testing::PrintToString(resulted.m_acknowledge);
    }

    if (expected.m_payload != resulted.m_payload)
    {
        return ::testing::AssertionFailure() << s.str() << " Payload differ: \n expected: "
                                             << ::testing::PrintToString(expected.m_payload)
                                             << "\n result: " << ::testing::PrintToString(resulted.m_payload);
    }

    return ::testing::AssertionSuccess();
}
