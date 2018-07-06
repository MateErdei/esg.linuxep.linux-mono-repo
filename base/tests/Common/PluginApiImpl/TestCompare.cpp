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

    if (expected.ProtocolVersion != resulted.ProtocolVersion)
    {
        return ::testing::AssertionFailure() << s.str() << " Protocol differ: \n expected: "
                                             << expected.ProtocolVersion
                                             << "\n result: " << resulted.ProtocolVersion;
    }

    if (expected.ApplicationId != resulted.ApplicationId)
    {
        return ::testing::AssertionFailure() << s.str() << " Application Id differ: \n expected: "
                                             << expected.ApplicationId
                                             << "\n result: " << resulted.ApplicationId;
    }

    if (expected.MessageId != resulted.MessageId)
    {
        return ::testing::AssertionFailure() << s.str() << " Message Id differ: \n expected: "
                                             << expected.MessageId
                                             << "\n result: " << resulted.MessageId;
    }

    if (expected.Command != resulted.Command)
    {
        return ::testing::AssertionFailure() << s.str() << " command differ: \n expected: "
                                             << Common::PluginProtocol::SerializeCommand(expected.Command)
                                             << "\n result: "
                                             << Common::PluginProtocol::SerializeCommand(resulted.Command);
    }

    if (expected.Error != resulted.Error)
    {
        return ::testing::AssertionFailure() << s.str() << " Error message differ: \n expected: "
                                             << expected.Error
                                             << "\n result: " << resulted.Error;
    }

    if (expected.Payload != resulted.Payload)
    {
        return ::testing::AssertionFailure() << s.str() << " Payload differ: \n expected: "
                                             << ::testing::PrintToString(expected.Payload)
                                             << "\n result: " << ::testing::PrintToString(resulted.Payload);
    }

    return ::testing::AssertionSuccess();
}
