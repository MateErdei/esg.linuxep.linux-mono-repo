// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/FileSystem/IFileSystem.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace Common::UtilityImpl
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
            return haystack.rfind(needle, 0) == 0;
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

        static bool isPositiveInteger(const std::string& number)
        {
            return !number.empty() &&
                   std::find_if(number.cbegin(), number.cend(), [](unsigned char c) { return !std::isdigit(c); }) ==
                       number.cend();
        }

        static std::vector<std::string> splitString(const std::string& originalstring, const std::string& separator)
        {
            std::vector<std::string> result;
            if (originalstring.empty())
            {
                result.push_back(std::string{});
                return result;
            }
            if (separator.empty())
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

        static std::string extractValueFromIniFile(const std::string& filePath, const std::string& key)
        {
            auto fs = Common::FileSystem::fileSystem();
            if (fs->isFile(filePath))
            {
                std::vector<std::string> contents = fs->readLines(filePath);
                for (auto const& line : contents)
                {
                    if (startswith(line, key + " ="))
                    {
                        std::vector<std::string> list = splitString(line, "=");
                        return list[1].erase(0, 1);
                    }
                }
                return "";
            }
            throw std::runtime_error("File doesn't exist :" + filePath);
        }
        static std::string extractValueFromConfigFile(const std::string& filePath, const std::string& key)
        {
            auto fs = Common::FileSystem::fileSystem();
            if (fs->isFile(filePath))
            {
                std::vector<std::string> contents = fs->readLines(filePath);
                for (auto const& line : contents)
                {
                    if (startswith(line, key + "="))
                    {
                        std::vector<std::string> list = splitString(line, "=");
                        return list[1];
                    }
                }
                return "";
            }
            throw std::runtime_error("File doesn't exist :" + filePath);
        }

        static bool isVersionOlder(const std::string& currentVersion, const std::string& newVersion)
        {
            // we are using this to check if the new version is older than the current version
            //  current version = 1.2   new/warehouse version = 1.3 -> false
            //  current version = 1.2   new/warehouse version = 1 -> true
            if (currentVersion.find_first_not_of("1234567890.") != std::string::npos ||
                newVersion.find_first_not_of("1234567890.") != std::string::npos)
            {
                throw std::invalid_argument(
                    "Invalid version data provided version" + currentVersion + ":" + newVersion);
            }

            if (currentVersion == newVersion)
            {
                return false;
            }

            std::string space = "";

            std::vector<std::string> version1 = splitString(currentVersion, ".");
            version1.erase(std::remove(version1.begin(), version1.end(), space), version1.end());

            std::vector<std::string> version2 = splitString(newVersion, ".");
            version2.erase(std::remove(version2.begin(), version2.end(), space), version2.end());

            int maxIndex = std::min(version1.size(), version2.size());

            for (int i = 0; i < maxIndex; i++)
            {
                std::pair<int, std::string> val1 = stringToInt(version1[i]);
                std::pair<int, std::string> val2 = stringToInt(version2[i]);

                if (val1.first > val2.first)
                {
                    return true;
                }
                if (val1.first < val2.first)
                {
                    return false;
                }
            }
            if (version1.size() > version2.size())
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
        static std::pair<int, std::string> stringToInt(const std::string& stringToConvert)
        {
            std::stringstream errorMessage;
            try
            {
                return std::make_pair(std::stoi(stringToConvert), "");
            }
            catch (const std::invalid_argument& e)
            {
                errorMessage << "Failed to find integer from output: " << stringToConvert
                             << ". Error message: " << e.what();
            }
            catch (const std::out_of_range& e)
            {
                errorMessage << "Failed to find integer from output: " << stringToConvert
                             << ". Error message: " << e.what();
            }

            return std::make_pair(0, errorMessage.str());
        }
        static std::pair<long, std::string> stringToLong(const std::string& stringToConvert)
        {
            std::stringstream errorMessage;
            try
            {
                return std::make_pair(std::stol(stringToConvert), "");
            }
            catch (const std::invalid_argument& e)
            {
                errorMessage << "Failed to find integer from output: " << stringToConvert
                             << ". Error message: " << e.what();
            }
            catch (const std::out_of_range& e)
            {
                errorMessage << "Failed to find integer from output: " << stringToConvert
                             << ". Error message: " << e.what();
            }

            return std::make_pair(0, errorMessage.str());
        }

        static unsigned long stringToULong(const std::string& stringToConvert)
        {
            try
            {
                return std::stoul(stringToConvert);
            }
            catch (const std::invalid_argument& e)
            {
                throw std::runtime_error("Failed to find unsigned long from output :" + stringToConvert + ". Error message: " + e.what());
            }
            catch (const std::out_of_range& e)
            {
                throw std::runtime_error("Value is out of range :" + stringToConvert + ". Error message: " + e.what());
            }
        }

        /*
         * Remove whitespace from the right side of a string.
         * Or if trimComparator is set then it will use that comparator to trim characters from the right.
         */
        static std::string rTrim(
            std::string string,
            const std::function<bool(char)>& trimComparator = [](char c) { return std::isspace(c); })
        {
            string.erase(
                std::find_if(
                    string.rbegin(), string.rend(), [trimComparator](char c) { return !trimComparator(c); })
                    .base(),
                string.end());
            return string;
        }

        /*
         * Remove whitespace from the left side of a string.
         * Or if trimComparator is set then it will use that comparator to trim characters from the left.
         */
        static std::string lTrim(
            std::string string,
            const std::function<bool(char)>& trimComparator = [](char c) { return std::isspace(c); })
        {
            string.erase(
                string.begin(),
                std::find_if(
                    string.begin(), string.end(), [trimComparator](char c) { return !trimComparator(c); }));
            return string;
        }

        /*
         * Remove whitespace from both sides of a string.
         * Or if trimComparator is set then it will use that comparator to trim characters from both sides.
         */
        static std::string trim(
            std::string string,
            const std::function<bool(char)>& trimComparator = [](char c) { return std::isspace(c); })
        {
            return lTrim(rTrim(string, trimComparator), trimComparator);
        }

        /*
         * Make all chars in string lowercase
         */
        static std::string toLower(std::string& string)
        {
            std::transform(string.begin(), string.end(), string.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            return string;
        }
    };
} // namespace Common::UtilityImpl