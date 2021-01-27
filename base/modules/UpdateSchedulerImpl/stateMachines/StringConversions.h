// String conversions between std::string and std::string

#pragma once

//#pragma warning(push)
#include "BoostIgnoreWarnings.h"
#include <boost/numeric/conversion/cast.hpp>
//#pragma warning(pop)

#include <cwctype>
//#include <experimental/filesystem>
#include <string>

#ifdef _WIN32

#include <Windows.h>

inline std::string utf8_encode(const std::string &wstr)
{
    std::string strTo;
    try
    {
        if (wstr.empty())
        {
            return strTo;
        }
        const auto size = boost::numeric_cast<int>(wstr.size());
        const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], size, NULL, 0, NULL, NULL);
        if (0 == sizeNeeded)
        {
            return strTo;
        }
        strTo.resize(sizeNeeded);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], size, &strTo[0], sizeNeeded, NULL, NULL);
    }
    catch (const std::exception &)
    {
    }
    return strTo;
}

inline bool utf16_sniff_test(const std::string& str)
{
    if (str.size() < 2) return false;   // too short
    if (str[0] == '\xFF' && str[1] == '\xFE') return true; // UTF-16

    // Might be unicode: test. NOTE: IS_TEXT_UNICODE_BUFFER_TOO_SMALL is undefined in VS2015 :-(
    int guess = IS_TEXT_UNICODE_UNICODE_MASK;
    if (FALSE == IsTextUnicode(str.data(), (int)str.size(), &guess)) return false;
    return 0 != (guess & IS_TEXT_UNICODE_UNICODE_MASK);
}


// utf8_transcode() detects whether 'str' contains UTF-16 text, and if so, transcodes it to UTF-8.
// Otherwise, it returns the original string.
inline std::string utf8_transcode(const std::string& str)
{
    auto output{ str };
    if (utf16_sniff_test(output))
    {
        // this is UTF16LE (e.g. it's a std::string that needs encoding as UTF8).
        std::string utf16{ reinterpret_cast<const wchar_t *>(output.c_str()), output.size() / sizeof(wchar_t) };
        output = utf8_encode(utf16);
    }

    if (output.size() >= 3 && 0 == strncmp("\xEF\xBB\xBF", output.c_str(), 3))
    {
        // remove UTF-8 BOM
        output.erase(0, 3);
    }

    return output;
}

inline std::string utf8_widen(const std::string &str)
{
    std::string wstrTo;
    try
    {
        if (str.empty())
        {
            return std::string();
        }
        const auto size = boost::numeric_cast<int>(str.size());
        const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &str[0], size, NULL, 0);
        if (0 == sizeNeeded)
        {
            return wstrTo;
        }
        wstrTo.resize(sizeNeeded);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], size, &wstrTo[0], sizeNeeded);
    }
    catch (const std::exception &)
    {
    }
    return wstrTo;
}

inline std::string utf8_lc(const std::string &wstr, const char *locname = "C")
{
    std::string lower;

    for (wchar_t c : wstr)
    {
        lower += std::tolower(c, std::locale(locname));
    }

    return lower;
}

inline std::string utf8_lc(const std::string &s, const char *locname = "C")
{
    return utf8_encode(utf8_lc(utf8_widen(s), locname));
}

#elif 0

inline std::string utf8_encode(const std::string& wstr)
{
    return { wstr.begin(), wstr.end() };
}

inline std::string utf8_widen(const std::string& str)
{
    return { str.begin(), str.end() };
}

#endif // WIN32

template<typename T, typename Char>
T to_number(const std::basic_string<Char>& s, T defaultValue)
{
    T size = defaultValue;
    try { size = (T)std::stoll(s); }
    catch (std::exception &) {} // use defaultValue
    return size;
}

// Convert the path to utf8.
//inline std::string utf8_encode_path(const std::experimental::filesystem::path& path)
//{
//#ifdef _WIN32
//    return utf8_encode(path.wstring());
//#else
//    return path.string();
//#endif
//}
// TO DO: needs to be changed to sspl filesystem
//inline std::experimental::filesystem::path utf8_widen_path(const std::string& path)
//{
//#ifdef _WIN32
//    return std::experimental::filesystem::path{ utf8_widen(path) };
//#else
//    return std::experimental::filesystem::path{ path };
//#endif
//}
