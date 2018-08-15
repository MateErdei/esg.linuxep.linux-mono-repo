/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <sstream>

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
             * Replace all instances of key with replace in pattern.
             * @param pattern Base string to do replacements in
             * @param key Target string to replace
             * @param replace Replacement string
             * @return pattern with all non-overlapping instances of key replaced with replace
             */
            static std::string replaceAll(const std::string& pattern, const std::string& key, const std::string& replace)
            {
                if (key.empty())
                {
                    return pattern;
                }
                std::string result;
                size_t beginPos = 0;

                while(true)
                {
                    size_t pos = pattern.find(key, beginPos);

                    if (pos == std::string::npos)
                    {
                        break;
                    }
                    result += pattern.substr(beginPos, pos - beginPos);
                    result += replace;
                    beginPos = pos + key.length();
                }

                result += pattern.substr(beginPos);

                return result;
            }
        };
    }
}