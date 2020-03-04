/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StringUtils.h"

void unixsocket::xmlEscape(std::string& text)
{
    std::string buffer;
    buffer.reserve(text.size());
    for(size_t pos = 0; pos != text.size(); ++pos) {
        switch(text[pos]) {
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
            case '\\': buffer.append("\\");            break;
            default: buffer.push_back(text[pos]);         break;
        }
    }

    text.swap(buffer);
}