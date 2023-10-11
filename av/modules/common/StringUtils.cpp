// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "StringUtils.h"

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

#include <boost/locale.hpp>

#include <cctype>
#include <ctime>
#include <iomanip>


namespace common
{
    void escapeControlCharacters(std::string& text, bool escapeXML)
    {
        std::string buffer;
        buffer.reserve(text.size());
        for(size_t pos = 0; pos != text.size(); ++pos) {
            switch(text[pos]) {
                // control characters
                case '\a': buffer.append("\\a");           break;
                case '\b': buffer.append("\\b");           break;
                case '\t': buffer.append("\\t");           break;
                case '\n': buffer.append("\\n");           break;
                case '\v': buffer.append("\\v");           break;
                case '\f': buffer.append("\\f");           break;
                case '\r': buffer.append("\\r");           break;
                case '\\': buffer.append("\\\\");          break;
                // xml special characters
                case '&':
                    if(escapeXML)
                    {
                        buffer.append("&amp;");
                        break;
                    }
                    [[fallthrough]];
                case '\"':
                    if(escapeXML)
                    {
                        buffer.append("&quot;");
                        break;
                    }
                    [[fallthrough]];
                case '\'':
                    if(escapeXML)
                    {
                        buffer.append("&apos;");
                        break;
                    }
                    [[fallthrough]];
                case '<':
                    if(escapeXML)
                    {
                        buffer.append("&lt;");
                     break;
                    }
                    [[fallthrough]];
                case '>':
                    if(escapeXML)
                    {
                        buffer.append("&gt;");
                        break;
                    }
                    [[fallthrough]];
                default:
                    unsigned int character = static_cast<unsigned char>(text[pos]);
                    if (character <= 31 || character == 127)
                    {
                        std::ostringstream escaped;
                        escaped << '\\'
                                << std::oct
                                << std::setfill('0')
                                << std::setw(3)
                                << character
                                ;
                        buffer.append(escaped.str());
                    }
                    else
                    {
                        buffer.push_back(text[pos]);
                    }
                    break;
            }
        }

        text.swap(buffer);
    }

    std::string toUtf8(const std::string& str, bool appendConversion, bool throws)
    {
        try
        {
            return boost::locale::conv::to_utf<char>(str, "UTF-8", boost::locale::conv::stop);
        }
        catch(const boost::locale::conv::conversion_error& e)
        {
            LOGDEBUG("Could not convert from: " << "UTF-8");
        }

        std::vector<std::string> encodings{"EUC-JP", "Shift-JIS", "SJIS", "Latin1"};
        for (const auto& encoding : encodings)
        {
            try
            {
                if (appendConversion)
                {
                    std::string encoding_info = " (" + encoding + ")";
                    return boost::locale::conv::to_utf<char>(str,
                                                             encoding,
                                                             boost::locale::conv::stop).append(encoding_info);
                }
                else
                {
                    return boost::locale::conv::to_utf<char>(str, encoding, boost::locale::conv::stop);
                }

            }
            catch(const boost::locale::conv::conversion_error& e)
            {
                LOGDEBUG("Could not convert from: " << encoding);
                continue;
            }
            catch(const boost::locale::conv::invalid_charset_error& e)
            {
                LOGDEBUG(e.what());
                continue;
            }
        }

        if(throws)
        {
            throw std::runtime_error(std::string("Failed to convert string to utf8: ") + str);
        }

        LOGDEBUG("Failed to convert string: "<< str << "to utf8 returning original");
        return str;
    }

    std::string fromLogLevelToString(const log4cplus::LogLevel& logLevel)
    {
        using sp = Common::Logging::SophosLogLevel;
        static std::vector<std::string> LogNames{ { "OFF" }, { "TRACE" }, { "DEBUG" }, { "INFO" },
                                                  { "SUPPORT" }, { "WARN" },  { "ERROR" } };

        static std::vector<sp> LogLevels{ sp::OFF, sp::TRACE, sp::DEBUG, sp::INFO, sp::SUPPORT, sp::WARN, sp::ERROR };

        auto ind_it = std::find(LogLevels.begin(), LogLevels.end(), logLevel);
        if (ind_it == LogLevels.end())
        {
            return std::string("Unknown (") + std::to_string(logLevel) + ")";
        }
        assert(std::distance(LogLevels.begin(), ind_it) >= 0);
        assert(std::distance(LogLevels.begin(), ind_it) < static_cast<int>(LogLevels.size()));
        return LogNames.at(std::distance(LogLevels.begin(), ind_it));
    }

    std::string pluralize(int number, std::string singularString, std::string pluralString)
    {
        if (number == 1)
        {
            return singularString;
        }
        else
        {
            return pluralString;
        }
    }

    std::string escapePathForLogging(const std::string& path, bool appendUtf8Conversion, bool Utf8ConversionThrows, bool escapeXmlCharacters)
    {
        std::string pathToEscape = toUtf8(path, appendUtf8Conversion, Utf8ConversionThrows);
        escapeControlCharacters(pathToEscape, escapeXmlCharacters);
        return pathToEscape;
    }

    bool contains(const std::string& string, const std::string& value)
    {
        return string.find(value, 0) != std::string::npos;
    }

    bool isStringHex(const std::string& stringToCheck)
    {
        for (char i : stringToCheck)
        {
            if (!isxdigit(i))
            {
                return false;
            }
        }
        return true;
    }

    std::string getSusiStyleTimestamp()
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        struct tm now_tm;
        gmtime_r(&now_tt, &now_tm);
        std::chrono::system_clock::duration timePointSinceEpoch = now.time_since_epoch();

        timePointSinceEpoch -= std::chrono::duration_cast<std::chrono::seconds>(timePointSinceEpoch);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timePointSinceEpoch).count();
        std::stringstream timestamp;
        timestamp << std::put_time(&now_tm, "%Y-%m-%dT%H:%M:%S.");
        timestamp << std::setfill('0') << std::setw(3);
        timestamp << ms << "Z";
        return timestamp.str();
    }
}

