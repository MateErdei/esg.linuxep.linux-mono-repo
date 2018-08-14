/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

// Originally from versig code

// iostr_utils.h
//   Some extra io manipulators for reading strings
//
//   Note that all these manipulators do not use lookahead
//   so if they fail, they do not wind back the stream to
//   the initial state.
//
//  20030902 Original code from DC version 1.0.0
//  20030922 RW
//           Changed to allow windows file separators to be included
//           in the pathnames of items within the body.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <sstream>

namespace std {

////////////////////////////////////////////////////////////
//
//  Various character classes
//

inline bool char_class_num(char c)      { return (c >= '0' && c <= '9'); }
inline bool char_class_alpha(char c)    { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
inline bool char_class_alphanum(char c) { return char_class_num(c) || char_class_alpha(c); }

inline bool char_class_base64(char c)   { return char_class_alphanum(c) || (c == '+') || (c == '/'); }
inline bool char_class_hex(char c)      { return char_class_num(c) || (c >= 'a' && c <= 'f'); }
//Note - this has been changed to allow the windows file separator as part of a filename.
inline bool char_class_file(char c)     { return (c >= 32 && c <= 126)
                              && (c != '"') && (c != ':')
                              && (c != '|') //&& (c != '\\')
                              && (c != '<') && (c != '>')
                              && (c != '*') && (c != '?');
                        }
inline bool char_literal_eq(char c)     { return (c == '='); }


////////////////////////////////////////////////////////////
// cin << expect("some constant string")
//
//  Read from the stream, if it does not match the expected
//  string then set the stream to a failed state.
//
struct _Expect_str { string s; };
inline _Expect_str expect (const string &s) {
    _Expect_str m;
    m.s = s;
    return m;
}

inline istream&
operator>>(istream& is, _Expect_str f)
{
    for (unsigned int n = 0; n != f.s.length(); n++) {
        char c;
        if (is.get(c) && (c != f.s[n])) {
            is.clear(ios::failbit);
            break;
        }
    }
    return is;
}


////////////////////////////////////////////////////////////
// cin << expect('c')
//
//  Read a single character from the stream, if it does not
//  match the expected character then set the stream to a failed
//  state.
//
struct _Expect_char { char c; };
inline _Expect_char expect (const char c) {
    _Expect_char m;
    m.c = c;
    return m;
}

inline istream&
operator>>(istream& is, _Expect_char f)
{
    char c;
    if (is.get(c) && (c != f.c))
        is.clear(ios::failbit);
    return is;
}


////////////////////////////////////////////////////////////
// cin << expect_eof
//
//  Require that no more characters are avaliable, or set stream
//  to failed state. So, if we are not at eof, we set fail, otherwise
//  the state will be that eof is set, but fail is not
//  ie the following are true: (is), (is.eof()), (!is.fail())
//
inline istream&
expect_eof(istream& is)
{
    //GNU & MS have different semantics here, hence the odd multi-stage test.

    if (is.eof()) return is;
    if (is.peek() == istream::traits_type::eof()) is.clear(ios::eofbit);
    if (!is.eof()) is.clear(ios::failbit);
    return is;
}


/////////////////////////////////////////////////////////////
// cin << match(character_predicate_function, result_string)
//
//  Reads a number of characters satisfying a character predicate
//  into a string from the stream. Optionally, If less than min
//  characters match or more than max matching chars are avaliable
//  then the stream is set to a failed state.
//
struct _Match {
    bool (*p) (char);
    string &s;
    int min, max;
    _Match (bool (*p) (char), string &s, int min, int max) :p(p), s(s), min(min), max(max) {}
};
inline _Match match (bool (*p) (char), string &s, int min = 0, int max = -2) {
    _Match m(p, s, min, max);
    return m;
}

inline istream&
operator>>(istream& is, _Match f)
{
    f.s = "";
    char c;
    int n = 0;
    while (is.get(c) && f.p(c) && ++n != f.max + 1) {   //NOTE: this is not an off by one error
        f.s += c;
    }
    if (is) is.putback(c);
    if (n < f.min || n == f.max + 1) is.clear(ios::failbit);
    return is;

    //the semantics of this is that we expect between min and max chars satisfying p
    //if there are more than max chars satisfying p, we fail (hence the need to read
    //one extra char to see if it matches)
}


////////////////////////////////////////////////////////////
// cin << match_base64(result_string)
//
//  Read a base64 encoding from a stream into a string.
//  Consumes (and requires) the trailing new line.
//
struct _Match_base64 {
    string &s;
    int maxlines;
    _Match_base64(string &s, int maxlines) : s(s), maxlines(maxlines) {}
};
inline _Match_base64 match_base64(string &s, int maxlines = -1) {
    _Match_base64 m(s, maxlines);
    return m;
}

inline istream&
operator>>(istream& is, _Match_base64 f)
{
    int lines;
    for (lines = 0; lines != f.maxlines; lines++) {
        string cur_line;
        is >> match(char_class_base64, cur_line, 0, 79);
        if (cur_line.length() == 0) break;
        f.s += cur_line;
        if (is.peek() == '\n') {
            f.s += '\n';
            is.ignore();
        } else if (is.peek() != '=') {
            is.clear(ios::failbit);
            break;
        };
        if (is.peek() == '=') {
            string padding;
            is >> match(char_literal_eq, padding, 1, 2) >> expect('\n');
            f.s += padding + "\n";
            break;
        };
    }
    if (lines == 0) is.clear(ios::failbit);
    return is;
}


////////////////////////////////////////////////////////////
// cin << get_upto(result_string, "delimiter string")
//
//  Read a string from a stream upto but not including a terminating
//  string. If maxbytes or eof is encountered first, the stream is set
//  to a failed state.
//
struct _Get_upto {
    string &s;
    const string &delim;
    int maxbytes;

    _Get_upto(string &s, const string &delim, int maxbytes):
        s(s), delim(delim), maxbytes(maxbytes) {}
};

inline _Get_upto get_upto(string &s, const string &delim, int maxbytes = -1)
{
    _Get_upto m(s, delim, maxbytes);
    return m;
}

inline istream& operator>>(istream& is, _Get_upto f)
{
    // the algorithm is to read through the stream char by char
    // and wait til the string we're looking for appears at the
    // end of our temp buffer
    // ie we're using lookbehind rather than lookahead.

    stringstream tmpbuf;
    size_t f_delim_len = f.delim.length();
    char f_delim_end = f.delim[f_delim_len - 1];
    unsigned long tmpstr_len;
    int terminator;

    //read into tmpbuf up to last character in the delim string
    while (is.good()) {
        is.get(*tmpbuf.rdbuf(), f_delim_end);

        if (! is.good())
        {
            // ifstream goes into fail state when writing blank lines into
            // tmpbuf.  According to docs, it can be cleared to continue.
            if (is.fail())
            {
                is.clear();
            }
            else
            {
                // Unknown state, break and continue to signature verification.
                break;
            }
        }

        //get returns EOF if there is nothing left to read.
        if (-1 == (terminator = is.get()))
        {
            continue;
        }

        //extract and store terminator
        tmpbuf.put(static_cast<char>(terminator));
        std::string tmpstr = tmpbuf.str();

        if ((tmpstr_len = tmpstr.length()) < f_delim_len)
        {
            continue;
        }

        string lookbehind = tmpstr.substr(
            (tmpstr_len - f_delim_len),
            f_delim_len
        );

        if (lookbehind == f.delim)
        {
            //found it
            f.s = tmpstr.substr(0, tmpstr_len - f_delim_len);
            return is;
        }
    }

    is.clear(ios::failbit);
    return is;
}

} // namespace std
