/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace Datatypes
    {
        class StringUtils
        {
        public:
            /**
             * Does s end with target
             * True if target is empty
             * @param s
             * @param target
             * @return
             */
            static inline bool endswith(const std::string& s, const std::string& target)
            {
                return s.rfind(target) == (s.size() - target.size());
            }
        };
    }
}