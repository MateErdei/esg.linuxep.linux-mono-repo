/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace UtilityImpl
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

            /**
             * Does s start with target
             * @param s
             * @param target
             * @return
             */
            static inline bool startswith(const std::string& s, const std::string& target)
            {
                return s.find(target) == 0;
            }

            /**
             * Replace all instances of target with replacement in s.
             * @param s
             * @param target
             * @return
             */
            static inline std::string replaceAll(std::string s, const std::string& target, const std::string& replacement)
            {
                if (target.empty())
                {
                    return s;
                }
                unsigned long pos = 0;
                while (true)
                {
                    pos = s.find(target,pos);
                    if (pos == std::string::npos)
                    {
                        break;
                    }
                    s = s.substr(0,pos) + replacement + s.substr(pos+target.size());
                    pos += replacement.size();
                }
                return s;
            }
        };
    }
}