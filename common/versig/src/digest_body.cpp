// digest_body.cpp: implementation of the digest_file_body class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_body.h"
#include "iostr_utils.h"
#include "print.h"
#include <limits>
#include <cassert>

namespace VerificationTool {

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

static void parseSHA256comment(const std::string& comment, file_info& info)
{
    assert(comment[0] == '#');
    if (comment.substr(1,7) != "sha256 ")
    {
        return;
    }
    std::string sha256 = comment.substr(8,72);
    info.setSha256(sha256);
}

istream& operator>>(istream &in, digest_file_body &v) {
    //parse the file body
    list<file_info> files;

    in >> noskipws;

    while (in.good()) {

        string path;
        unsigned long size;
        string checksum;
        char comment[100];
        switch (in.peek()) {	// With an MS iostream, calling peek() on a stream which is in an eof state
                                // sets the fail bit! So be careful to only call peek() once. GNU does it ok.
        case '#':
            // Need to work out if we have a sha-256 comment
            in.get(comment,100,'\n');
            if (in.gcount() == 72 and !files.empty())
            {
                // Possibly SHA-256 comment
                parseSHA256comment(comment,files.back());
            }
            else if (in.fail())
            {
                // comment longer than 100 bytes - never a SHA-256 comment.
                in.clear(ios::failbit);
            }
            // Either a single \n or comment longer than 100 bytes.
            in.ignore(numeric_limits<int>::max(), '\n');
            continue;

        case '"':
            in >> expect('"') >> match(char_class_file, path) >> expect('"') >> expect(' ')
               >> size >> expect(' ')
               >> match(char_class_hex,  checksum) >> expect('\n');
#if 03 == CPPSTD
            files.push_back(file_info(path, size, checksum));
#else
            files.emplace_back(path, size, checksum);
#endif
            continue;

        default:
            break;
        }
        break;	// can't break in the default branch, so break here and other branches continue
    };
    in >> expect_eof;

    if (in)
        v._files = files;

    return in;
}

} // namespace VerificationTool
