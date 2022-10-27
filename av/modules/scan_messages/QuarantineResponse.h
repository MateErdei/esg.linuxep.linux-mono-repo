// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <string>
#include <vector>
#include "common/CentralEnums.h"
#include "QuarantineResponse.capnp.h"

namespace scan_messages
{

    class QuarantineResponse
    {
    public:
        QuarantineResponse(common::CentralEnums::QuarantineResult result);
        explicit QuarantineResponse(Sophos::ssplav::QuarantineResponseMessage::Reader reader);

        [[nodiscard]] common::CentralEnums::QuarantineResult getResult();

        [[nodiscard]] std::string serialise() const;


    private:
        common::CentralEnums::QuarantineResult m_result;

    };
}

