// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

namespace unixsocket::updateCompleteSocket
{
    class IUpdateCompleteCallback
    {
    public:
        virtual ~IUpdateCompleteCallback() = default;
        virtual updateComplete() = 0;
    };
}
