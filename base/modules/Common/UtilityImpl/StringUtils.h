/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>

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

            static std::string replaceAll(const std::string& pattern, const std::string& key, const std::string& replace)
            {
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

            static std::vector<std::string> splitString( const std::string & originalstring, const std::string & separator)
            {
                std::vector<std::string> result;
                size_t beginPos = 0;

                while(true)
                {
                    size_t pos = originalstring.find(separator, beginPos);

                    if (pos == std::string::npos)
                    {
                        break;
                    }
                    result.emplace_back(originalstring.substr(beginPos, pos - beginPos));
                    beginPos = pos + separator.length();
                }

                result.emplace_back(originalstring.substr(beginPos));

                return result;
            }

            using KeyValueCollection = std::vector<std::pair<std::string, std::string>>;

            static std::string orderedStringReplace(const std::string& pattern, const KeyValueCollection& keyvalues)
            {
                std::string result;
                size_t beginPos = 0;

                for (auto & keyvalue : keyvalues)
                {
                    auto & key = keyvalue.first;
                    size_t pos = pattern.find(key, beginPos);
                    if (pos == std::string::npos)
                    {
                        break;
                    }
                    result += pattern.substr(beginPos, pos-beginPos);
                    result += keyvalue.second;
                    beginPos = pos + key.length();
                }

                result += pattern.substr(beginPos);
                return result;
            }

        };

    }
}