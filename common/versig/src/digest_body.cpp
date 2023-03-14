// digest_body.cpp: implementation of the digest_file_body class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_body.h"

#include "iostr_utils.h"
#include "print.h"

#include <cassert>
#include <limits>
#include <regex>
#include <vector>

namespace VerificationTool
{
    using namespace std;

    // digest file format / grammar
    //
    // Note this is a character level specification; there is no implied white space.
    // This grammar should have the property that a single character change to the file
    // should cause the parsing or verification to fail.
    //
    //	<digest file body>	::= (<file entry>|<comment>|<SHA-256 comment>)*
    //	<file entry>		::= <file name><file size><checksum><LF>
    //	<file name>		::= <quote><file>*<quote><space>
    //	<file size>		::= <num>*<space>
    //	<checksum>		::= <hex>*
    //	<SHA-256 comment>	::= <hash>sha256 <SHA256Hash>
    // 	<SHA256Hash> 	::= <hex>{64}
    //	<comment>		::= <hash><any>*<LF>
    //
    //	<space>	::= ' '
    //	<dot>	::= '.'
    //	<hash>	::= '#'
    //	<quote>	::= '"'
    //	<LF>	::= '\n'        (that is chr(10))
    //	<equal>	::= '='
    //	<any>	::= .		(that is matches any single character)
    //
    //	<num>	   = [0-9]
    //	<alphanum> = [0-9,a-z,A-Z]
    //	<base64>   = [0-9,a-z,A-Z,'+','/']
    //	<file>	   = [\32-\126,^'"',':','|','\'<','>','*','?']
    //	<hex>	   = [0-9,a-f]
    //

    static std::regex shaXXX_regex("#(sha256|sha384) ([a-f0-9]{64,})");

    // Parse comment lines. Comment lines may contain a keyword followed by arguments,
    // or they contain regular comments, which are ignored. Unknown keywords are ignored
    // to ensure forwards-compatibility.
    static void parse_comment_line(istream& in, list<file_info>& files)
    {
        using namespace std;
        vector<char> buffer(65536);

        in.getline(&buffer[0], static_cast<streamsize>(buffer.size()));

        // Comment lines apply to the last file_info; if there isn't one, ignore this line.
        if (files.empty())
            return;

        file_info& last_file = files.back();
        string comment(&buffer[0]);
        smatch match;

        // Look for SHAXXX attribute and save it
        if (regex_match(comment, match, shaXXX_regex) && match.size() == 3)
        {
            if (match[1] == "sha256")
                last_file.sha256(match[2]);
            else if (match[1] == "sha384")
                last_file.sha384(match[2]);
        }
    }

    istream& operator>>(istream& in, digest_file_body& v)
    {
        // parse the file body
        list<file_info> files;

        in >> noskipws;

        while (in.good())
        {
            string path;
            unsigned long size;
            string checksum;
            char comment[100];
            switch (in.peek())
            { // With an MS iostream, calling peek() on a stream which is in an eof state
              // sets the fail bit! So be careful to only call peek() once. GNU does it ok.
                case '#':
                    parse_comment_line(in, files);
                    continue;

                case '"':
                    in >> expect('"') >> match(char_class_file, path) >> expect('"') >> expect(' ') >> size >>
                        expect(' ') >> match(char_class_hex, checksum) >> expect('\n');
#if 03 == CPPSTD
                    files.push_back(file_info(path, size, checksum));
#else
                    files.emplace_back(path, size, checksum);
#endif
                    continue;

                default:
                    break;
            }
            break; // can't break in the default branch, so break here and other branches continue
        };
        in >> expect_eof;

        if (in)
            v._files = files;

        return in;
    }

} // namespace VerificationTool
