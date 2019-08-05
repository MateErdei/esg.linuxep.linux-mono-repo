/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <sstream>
#include <string.h>
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
             * Does s contains target
             * @param s
             * @param target
             * @return
             */
            static inline bool isSubstring(const std::string& s, const std::string& target)
            {
                return s.find(target) == std::string::npos;
            }

            /**
             * Replace all instances of key with replace in pattern.
             * @param pattern Base string to do replacements in
             * @param key Target string to replace
             * @param replace Replacement string
             * @return pattern with all non-overlapping instances of key replaced with replace
             */
            static std::string replaceAll(
                const std::string& pattern,
                const std::string& key,
                const std::string& replace)
            {
                if (key.empty())
                {
                    return pattern;
                }
                std::string result;
                size_t beginPos = 0;

                while (true)
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

            static std::vector<std::string> splitString(const std::string& originalstring, const std::string& separator)
            {
                std::vector<std::string> result;
                size_t beginPos = 0;

                while (true)
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

            // coverity [ +tainted_string_sanitize_content : arg-0 ]
            static std::string checkAndConstruct(const char* untaintedCString, size_t maxLen = 10000)
            {
                if (::strnlen(untaintedCString, maxLen) == maxLen)
                {
                    throw std::invalid_argument{ "Input c-string exceeds a reasonable limit " };
                }
                return std::string{ untaintedCString };
            }

            using KeyValueCollection = std::vector<std::pair<std::string, std::string>>;

            /**
             * Ordered String Replace allows a very efficient string pattern replace by enforcing that the keys come in
             * the order that they must be replaced in the pattern.
             * @param pattern: string containing the template with the keys to be replaced by the values. For example:
             * Hello name. Good greeting!
             * @param keyvalues: vector of pairs containing key|value. For example: {{"name",
             * "sophos"},{"greeting","morning"}}
             * @return The string where keys were replaced for their respective value: For example: Hello sophos. Good
             * morning!
             *
             * @attention If the keyvalues are given in the incorrect value the keys will not be replaced!
             *
             * For example: pattern = Hello name. Good greeting!
             * keyvalues = {{"greeting","morning"}, {"name", "sophos"}}
             * Result = Hello name. Good morning!
             *              *
             */
            static std::string orderedStringReplace(const std::string& pattern, const KeyValueCollection& keyvalues)
            {
                std::string result;
                size_t beginPos = 0;

                for (auto& keyvalue : keyvalues)
                {
                    auto& key = keyvalue.first;
                    size_t pos = pattern.find(key, beginPos);
                    if (pos == std::string::npos)
                    {
                        break;
                    }
                    result += pattern.substr(beginPos, pos - beginPos);
                    result += keyvalue.second;
                    beginPos = pos + key.length();
                }

                result += pattern.substr(beginPos);
                return result;
            }
        };

    } // namespace UtilityImpl
} // namespace Common