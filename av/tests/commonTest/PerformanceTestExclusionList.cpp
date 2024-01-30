// Copyright 2023 Sophos Limited. All rights reserved.

#include "common/ExclusionList.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Logging/SophosLoggerMacros.h"
#include "Common/Main/Main.h"

#include "re2/re2.h"

#include <chrono>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#define LOGNAME "PerfTest"
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(LOGNAME, x)
#define LOGINFO(x)  LOG4CPLUS_INFO(LOGNAME, x)
#define LOGERROR(x) LOG4CPLUS_ERROR(LOGNAME, x)

namespace
{
    class ExclusionAccessor : public common::Exclusion
    {
    public:
        static std::string convertGlobToRegexStringExposed(const common::CachedPath& glob)
        {
            return convertGlobToRegexString(glob);
        }
    };

    std::string regexFromGlob(const std::string& glob)
    {
        return ExclusionAccessor::convertGlobToRegexStringExposed(common::CachedPath{glob});
    }

    std::vector<std::string> readLines(const std::string& file)
    {
        std::vector<std::string> results;
        std::ifstream infile{file};
        std::string line;
        while (std::getline(infile, line))
        {
            results.push_back(line);
        }
        return results;
    }

    // Force the compiler to actually run the function
    template<typename T>
    void use(T &&t) {
        __asm__ __volatile__ ("" :: "g" (t));
    }

    void testExclusionList(const std::vector<std::string>& exclusionStrings, const std::vector<common::CachedPath>& testPaths)
    {
        common::ExclusionList exclusions{exclusionStrings};
        if (exclusions.size() == 0)
        {
            LOGERROR("NO EXCLUSIONS!");
        }

        int globExclusions = 0;

        for (unsigned i=0; i<exclusions.size(); i++)
        {
            if (exclusions.at(i).isGlobExclusion())
            {
                globExclusions++;
            }
//            LOGDEBUG("type:" << exclusions.at(i).type());
        }

        LOGINFO("Starting test: "
                        << exclusions.size() << " exclusions, "
                        << globExclusions << " globs, "
                        << testPaths.size() << " test paths");
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& test: testPaths)
        {
            auto result = exclusions.appliesToPath(test, false, true);
            use(result);
        }
        auto end = std::chrono::high_resolution_clock::now();
        LOGINFO("Finished test");

        auto duration = end - start;

        LOGINFO("Duration = " << std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
                              << " micro-seconds");
    }

    std::string combinedRegexStr(const std::vector<std::string>& exclusionStrings)
    {
        std::string regexString;
        for (const auto& excl : exclusionStrings)
        {
            if (!regexString.empty())
            {
                regexString.push_back('|');
            }
            regexString += regexFromGlob(excl);
        }
        LOGDEBUG("Regex String: " << regexString);
        return regexString;
    }

    bool regexMatch(const RE2& regex, const common::CachedPath& test)
    {
        return RE2::FullMatch(test.c_str(), regex);
    }

    bool regexMatch(const std::regex& regex, const common::CachedPath& test)
    {
        return std::regex_match(test.c_str(), regex);
    }

    template<class regex_t>
    void testMultiGlob(const regex_t& regex, const std::vector<common::CachedPath>& testPaths)
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& test: testPaths)
        {
            auto result = regexMatch(regex, test);
            use(result);
        }
        auto end = std::chrono::high_resolution_clock::now();
        LOGINFO("Finished test");

        auto duration = end - start;

        LOGINFO("Duration = " << std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
                              << " micro-seconds");
    }

    void testMultiGlobRE2(const std::vector<std::string>& exclusionStrings, const std::vector<common::CachedPath>& testPaths)
    {
        auto regexString = combinedRegexStr(exclusionStrings);
        LOGINFO("Starting RE2 test: "
                        << exclusionStrings.size() << " exclusions, "
                        << "in one regex of length " << regexString.size() << ", "
                        << testPaths.size() << " test paths");

        RE2 regex{regexString};
        testMultiGlob(regex, testPaths);
    }

    void testMultiGlobStdRegex(const std::vector<std::string>& exclusionStrings, const std::vector<common::CachedPath>& testPaths)
    {
        auto regexString = combinedRegexStr(exclusionStrings);
        LOGINFO("Starting std::regex test: "
                        << exclusionStrings.size() << " exclusions, "
                        << "in one regex of length " << regexString.size() << ", "
                        << testPaths.size() << " test paths");

        std::regex regex{regexString};
        testMultiGlob(regex, testPaths);
    }

    int inner_main(int argc, char* argv[])
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
        if (argc < 3)
        {
            LOGINFO("./PerformanceTestExclusionList <exclusions-file> <test-paths-file>");
            return 1;
        }
        std::vector<std::string> exclusionStrings = readLines(argv[1]);
        std::vector<std::string> testStrings = readLines(argv[2]);
        std::vector<common::CachedPath> testPaths;
        testPaths.reserve(testStrings.size());
        for (const auto& p : testStrings)
        {
            testPaths.emplace_back(p);
        }
        testExclusionList(exclusionStrings, testPaths);
        if (argc > 3)
        {
            testMultiGlobRE2(exclusionStrings, testPaths);
            testMultiGlobStdRegex(exclusionStrings, testPaths);
        }

        return 0;
    }
}

MAIN(inner_main(argc, argv));
