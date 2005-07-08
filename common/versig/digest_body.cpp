// digest_body.cpp: implementation of the digest_file_body class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_body.h"
#include "iostr_utils.h"
#include <limits>

namespace VerificationTool {

using namespace std;

// digest file format / grammar
//
// Note this is a character level specification; there is no implied white space.
// This grammar should have the property that a single character change to the file
// should cause the parsing or verification to fail.
//
//	<digest file body>	::= (<file entry>|<comment>)*
//	<file entry>		::= <file name><file size><checksum><LF>
//	<file name>		::= <quote><file>*<quote><space>
//	<file size>		::= <num>*<space>
//	<checksum>		::= <hex>*
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

istream& operator>>(istream &in, digest_file_body &v) {
	//parse the file body
	list<file_info> files;
	
	in >> noskipws;
	
	while (in.good()) {
		
		switch (in.peek()) {	// With an MS iostream, calling peek() on a stream which is in an eof state
								// sets the fail bit! So be careful to only call peek() once. GNU does it ok.
		case '#':
			in.ignore(numeric_limits<int>::max(), '\n');
			continue;

		case '"':
			string path;
			unsigned long size;
			string checksum;
			in >> expect('"') >> match(char_class_file, path) >> expect('"') >> expect(' ')
			   >> size >> expect(' ')
			   >> match(char_class_hex,  checksum) >> expect('\n');
			files.push_back(file_info(path, size, checksum));
			continue;
		}
		break;	// can't break in the default branch, so break here and other branches continue
	};
	in >> expect_eof;
	
	if (in)
		v._files = files;
	
	return in;
}

} // namespace VerificationTool
