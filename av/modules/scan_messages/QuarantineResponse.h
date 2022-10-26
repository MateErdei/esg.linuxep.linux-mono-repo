// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>
#include <vector>

#include "QuarantineResponse.capnp.h"

namespace scan_messages
{
    enum QuarantineResult : int
    {
        QUARANTINE_SUCCESS = 0,
        QUARANTINE_FAIL = 1,
        QUARANTINE_DELETE = 2,
        QUARANTINE_FAIL_TO_DELETE_FILE = 3,
        QUARANTINE_WHITELISTED = 4,
    };

    class QuarantineResponse
    {
    public:
        QuarantineResponse(QuarantineResult result);
        explicit QuarantineResponse(Sophos::ssplav::QuarantineResponseMessage::Reader reader);

        [[nodiscard]] QuarantineResult getResult();

        [[nodiscard]] std::string serialise() const;


    private:
        QuarantineResult m_result;

    };
}

