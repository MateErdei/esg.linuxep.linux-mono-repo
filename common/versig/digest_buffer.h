// digest_buffer.h : interface for the digest_file_buffer class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DIGEST_FILE_BUFFER_H_INCLUDED_)
#define _DIGEST_FILE_BUFFER_H_INCLUDED_

#include <iostream>
#include <string>
#include <list>

namespace VerificationTool {

using namespace std;

class digest_file_buffer {
private:
	unsigned long	_file_buf_max;	// a maximum size to read into the _file_buf, to stop DOS attacks (default 128k)
	string			_file_buf;		// buffer containing the bit of the file over which the signature is calculated
	string			_signature;		// the digital signature at the end of the file	
	string			_certificate;	// the certificate used to sign the file
	list<string>	_cert_chain;	// the chain of certificates needed to verify the file's authenticity

public:
	digest_file_buffer(): _file_buf_max(1024 * 128) {}
	void set_file_body_limit(unsigned long lim) { _file_buf_max = lim; }
	
	const string&		file_body()   const { return _file_buf; }
	const string&		signature()   const { return _signature; }
	const string&		certificate() const { return _certificate; }
	const list<string>&	cert_chain()  const { return _cert_chain; }

	friend istream& operator>>(istream &s, digest_file_buffer &v);
};

} // namespace VerificationTool

// re-enable warning about very long type identifiers in debug info
//#pragma warning (default : 4786)

#endif // !defined(_DIGEST_FILE_BUFFER_H_INCLUDED_)
