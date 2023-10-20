// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

#include "tests/commonTest/pluginutils/TestAbstractThreadImpl.h"

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockUpdateCompleteServerSocket : public unixsocket::updateCompleteSocket::UpdateCompleteServerSocket
    {
        public:
            MockUpdateCompleteServerSocket(const sophos_filesystem::path _serverPath, mode_t _mode)
                : unixsocket::updateCompleteSocket::UpdateCompleteServerSocket(_serverPath, _mode)
                {
                };

            MOCK_METHOD(void, updateComplete, (), (override));
    };
}