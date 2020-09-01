/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <sstream>
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <Common/FileSystem/IFileSystem.h>


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
             * Does haystack starts with needle
             * @param haystack
             * @param needle
             * @return
             */
            static inline bool startswith(const std::string& haystack, const std::string& needle)
            {
                return haystack.find(needle) == 0;
            }

            /**
             * Does s contains target
             * @param s
             * @param target
             * @return
             */
            static inline bool isSubstring(const std::string& s, const std::string& target)
            {
                return s.find(target) != std::string::npos;
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
                if( originalstring.empty())
                {
                    result.push_back(std::string{});
                    return result;
                }
                if( separator.empty())
                {
                    result.emplace_back(originalstring);
                    return result;
                }
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
                if (untaintedCString == nullptr)
                {
                    return "";
                }

                if (::strnlen(untaintedCString, maxLen) == maxLen)
                {
                    throw std::invalid_argument{ "Input c-string exceeds a reasonable limit " };
                }
                enforceUTF8(untaintedCString);

                return std::string{ untaintedCString };
            }

            static void enforceUTF8(const std::string& input);

            static std::optional<std::string> extractValueFromIniFile(const std::string& filePath, const std::string& key)
            {
                auto fs = Common::FileSystem::fileSystem();
                if (fs->isFile(filePath))
                {
                    std::vector<std::string> contents = fs->readLines(filePath);
                    for (auto const &line: contents)
                    {
                        if (startswith(line,key))
                        {
                            std::vector<std::string> list = splitString(line,"=");
                            return list[1];
                        }
                    }
                }
            }
            bool isVersionOlder(const std::string& currentVersion, const std::string& newVersion)
            {
                if( (currentVersion.find_first_not_of("1234567890.") != std::string::npos ) ||
                     (newVersion.find_first_not_of("1234567890.") != std::string::npos ))
                {
                    throw std::runtime_error("Invalid version data provided version" + currentVersion + ":" + newVersion );
                }

                if(currentVersion == newVersion)
                {
                    return false;
                }

                std::vector<std::string> version1 = splitString(currentVersion,".");
                std::vector<std::string> version2 = splitString(newVersion,".");

                int maxIndex = std::min(version1.size(), version2.size());

                for (int i = 0; i < maxIndex; i++)
                {
                    if (std::stoi(version1[i]) > std::stoi(version2[i]))
                    {
                        return true;
                    }
                }
                if (version2.size() > version1.size())
                {
                    return true;
                }
                return false;
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