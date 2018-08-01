/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "RegexUtilities.h"

namespace Common
{
    namespace UtilityImpl
    {

        std::string returnFirstMatch(const std::string& stringpattern, const std::string& content)
        {
            std::regex regexPattern{stringpattern};
            std::string result = returnFirstMatch(regexPattern, content);
            if (result.empty())
            {
                int a = 0;
            }
            return result;
        }

        std::string returnFirstMatch(const std::regex& regexPattern, const std::string& content)
        {
            std::smatch matches;

            if (std::regex_search(content.begin(), content.end(), matches, regexPattern))
            {
                if (1 < matches.size() && matches[1].matched)
                {
                    return matches[1];
                }
            }
            return std::string();
        }


    }

}