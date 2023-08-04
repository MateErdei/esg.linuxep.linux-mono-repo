// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace SulDownloader
{
    class SusResponseParseException : public Common::Exceptions::IException
    {
        using Common::Exceptions::IException::IException;
    };
} // namespace SulDownloader
