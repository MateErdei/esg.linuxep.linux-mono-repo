// digest_buffer_checker.cpp: implementation of the
// digest_buffer_checker class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#include "digest_buffer_checker.h"
#include "crypto_utils.h"
#include "verify_exceptions.h"
#include <sstream>
#include <assert.h>

namespace VerificationTool {

using namespace VerificationToolCrypto;
using namespace verify_exceptions;

//These 2 classes are used to automate cleanup of certificates if an
//exception is thrown
class CertFreer {
   X509* m_Cert;
public:
   CertFreer( X509 *cert ) : m_Cert(cert) {}
   ~CertFreer() {
      if (m_Cert) {
         X509_free(m_Cert);
      }
   }
};

class CertListFreer {
   list<X509*> m_List;
public:
   CertListFreer( list<X509*> list ) : m_List(list) {}
   ~CertListFreer() {
      for ( list<X509*>::iterator i = m_List.begin(); i != m_List.end(); i++ ){
         X509_free(*i);
      }
   }
};

class KeyFreer {
   EVP_PKEY* m_Key;
public:
   KeyFreer( EVP_PKEY* key ) : m_Key(key) {}
   ~KeyFreer() {
      if ( m_Key ){
         EVP_PKEY_free( m_Key );
      }
   }
};

bool digest_buffer_checker::verify_all(const string &trusted_certs_file, const string &crl_file) {

	//make X509 object from pem encoding
	X509 *cert = X509_decode(certificate());
   if ( !cert ){
      throw ve_crypt("Decoding certificate");
   }
   CertFreer FreeCert(cert);

	//extract public key from certificate
	EVP_PKEY *pubkey = X509_get_pubkey(cert);
   if ( !pubkey ){
      throw ve_crypt("Getting public key from certificate");
   }
   KeyFreer FreeKey(pubkey);

	//verify the signature against the file body using the public key
	istringstream file_body_stream(file_body());
	verify_signature(file_body_stream, base64_decode(signature()), pubkey); // throws

	//make X509 objects from pem encoding for cert chain
	list<X509 *> intermediate_cert_chain;
   CertListFreer cert_chain_freers(intermediate_cert_chain);
   for (list<string>::const_iterator p = cert_chain().begin(); p != cert_chain().end(); p++){
      X509* CurrentCert = X509_decode(*p);
      if (!CurrentCert){
         throw ve_crypt("Decoding certificate in chain");
      }
		intermediate_cert_chain.push_front(CurrentCert);
   }

	//verify the signing cert's path to a trusted cert
	verify_certificate_path(cert, trusted_certs_file, intermediate_cert_chain, crl_file); // throws

    return true;
}

} // namespace VerificationTool.Crypto

// vim:ts=4
