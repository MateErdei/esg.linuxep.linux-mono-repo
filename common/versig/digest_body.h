// digest_body.h: interface for the digest_file_body class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DIGEST_FILE_BODY_H_INCLUDED_)
#define _DIGEST_FILE_BODY_H_INCLUDED_

#include <iostream>
#include <string>
#include <list>

namespace VerificationTool {

using namespace std;

//This class represents the informatino about a file
class file_info {
	string _path;		// relative path and name
	unsigned long _size;	// size of the file in bytes
	string _checksum;	// ansii hex representation of sha1 checksum of the file

public:
	string path() const { return _path; };
	unsigned long size() const { return _size; };
	string checksum() const { return _checksum; };

	typedef enum { file_ok, file_invalid, file_missing } verify_result;

	verify_result verify_file(const string& root_path) const;

	file_info(string path, unsigned long size, string checksum): _path(path), _size(size), _checksum(checksum) {};
};

//This class represents the body of a digest file.
class digest_file_body {
private:
	
	list<file_info> _files;	// file info for each file
	
public:
	typedef list<file_info>::const_iterator files_iter;
		
	// begin & end iterators for the collection of files
	files_iter files_begin() const { return _files.begin(); };
	files_iter files_end() const { return _files.end(); };
	
	friend istream& operator>>(istream &s, digest_file_body &v);
};

} // namespace VerificationTool

// re-enable warning about very long type identifiers in debug info
//#pragma warning (default : 4786)

#endif // !defined(_DIGEST_FILE_BODY_H_INCLUDED_)
