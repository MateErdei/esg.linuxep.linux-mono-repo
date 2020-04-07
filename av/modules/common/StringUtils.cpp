/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <iomanip>
#include <openssl/sha.h>
#include <boost/locale.hpp>
#include "Common/UtilityImpl/StringUtils.h"
#include "ThreatDetected.capnp.h"
#include "Logger.h"
#include "StringUtils.h"

namespace common
{
    void escapeControlCharacters(std::string& text)
    {
        std::string buffer;
        buffer.reserve(text.size());
        for(size_t pos = 0; pos != text.size(); ++pos) {
            switch(text[pos]) {
                // control characters
                case '\1': buffer.append("\\1");           break;
                case '\2': buffer.append("\\2");           break;
                case '\3': buffer.append("\\3");           break;
                case '\4': buffer.append("\\4");           break;
                case '\5': buffer.append("\\5");           break;
                case '\6': buffer.append("\\6");           break;
                case '\a': buffer.append("\\a");           break;
                case '\b': buffer.append("\\b");           break;
                case '\t': buffer.append("\\t");           break;
                case '\n': buffer.append("\\n");           break;
                case '\v': buffer.append("\\v");           break;
                case '\f': buffer.append("\\f");           break;
                case '\r': buffer.append("\\r");           break;
                case '\\': buffer.append("\\\\");          break;
                // xml special characters
                case '&':  buffer.append("&amp;");         break;
                case '\"': buffer.append("&quot;");        break;
                case '\'': buffer.append("&apos;");        break;
                case '<':  buffer.append("&lt;");          break;
                case '>':  buffer.append("&gt;");          break;
                default:
                    auto character = static_cast<unsigned>(text[pos]);
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

    std::string sha256_hash(const std::string& str)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, str.c_str(), str.size());
        SHA256_Final(hash, &sha256);
        std::stringstream ss;
        for(const auto& ch : hash)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)ch;
        }
        return ss.str();
    }

    std::string toUtf8(const std::string& str)
    {
        try
        {
            return boost::locale::conv::to_utf<char>(str, "UTF-8", boost::locale::conv::stop);
        }
        catch(const boost::locale::conv::conversion_error& e)
        {
            LOGERROR("Could not convert from: " << "UTF-8");
        }

        std::vector<std::string> encodings{"EUC-JP", "SJIS", "Latin1"};
        for (const auto& encoding : encodings)
        {
            try
            {
                std::string encoding_info = " (" + encoding + ")";
                return boost::locale::conv::to_utf<char>(str, encoding, boost::locale::conv::stop).append(encoding_info);
            }
            catch(const boost::locale::conv::conversion_error& e)
            {
                LOGERROR("Could not convert from: " << encoding);
                continue;
            }
        }

        throw std::runtime_error(std::string("Failed to convert string to utf8: ") + str);
    }
}

