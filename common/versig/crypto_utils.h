// crypto_utils.h: interface for the cryptographic verification 
// functions.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CRYPTO_H_INCLUDED_)
#define _CRYPTO_H_INCLUDED_

#include <string>
#include <list>
#include <iostream>

#include <cstdio>

// uses the OpenSSL library
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

namespace VerificationToolCrypto {

using namespace std;

class CleanUpStack {
   STACK_OF(X509) * m_Stack;
public:
   CleanUpStack( STACK_OF(X509) * Stack ) : m_Stack(Stack) {}
   ~CleanUpStack() {
      sk_X509_pop_free(m_Stack, X509_free);
   }
};

bool verify_signature(istream &body,
		      const string &signature,
		      EVP_PKEY *pubkey);

bool verify_certificate_path(X509 *cert,
			     const string &trusted_certs_file,
			     const list<X509*> &untrusted_certs,
			     const string &crl_file = "");

X509* X509_decode(const string&);

string base64_decode(const string&);

string sha1sum(istream &);

// This function is used to build a human-readable
// error string from a stor context during the verification
// of a certificate chain.
//
string MakeErrString(X509_STORE_CTX* stor);

} // namespace VerificationToolCrypto

#endif // !defined(_CRYPTO_H_INCLUDED_)
