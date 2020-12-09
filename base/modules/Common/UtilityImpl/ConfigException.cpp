/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ConfigException.h"

#include <sstream>
namespace
{
    std::string combineMessages(const std::string& where, const std::string& cause)
    {
        std::stringstream s;
        s << "Failed to configure " << where << ". Reason: " << cause;
        return s.str();
    }
} // namespace

namespace Common
{
    namespace UtilityImpl
    {
        ConfigException::ConfigException(const std::string& where, const std::string& cause) :
            ConfigException(combineMessages(where, cause))
        {
        }
    } // namespace UtilityImpl

} // namespace Common