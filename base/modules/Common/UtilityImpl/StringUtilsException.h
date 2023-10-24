#pragma once

#include "Common/Exceptions/IException.h"

namespace Common::UtilityImpl
{
    class StringUtilsException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
} // namespace Common::UtilityImpl