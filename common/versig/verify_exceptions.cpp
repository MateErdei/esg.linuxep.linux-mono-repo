// verify_exceptions.cpp. This file implements exception and helper 
// classes for error handling within the verify library.
//
//  20030909 RW version 1.0.0
//
//////////////////////////////////////////////////////////////////////
#include "verify_exceptions.h"
#include <iostream>
#include <openssl/err.h>
#include <openssl/ssl.h>

using namespace std;

namespace verify_exceptions {

// Pop the next crypto error off the stack of OpenSSL
// crypto errors.
// Return true if an error was found, false otherwise.
//
bool ve_crypt::getNextCryptErr( string& ErrorDesc ) const {
   static bool StringsLoaded = false;
   if ( !StringsLoaded ){
      //We should only need to do this once.
      ERR_load_crypto_strings();
      StringsLoaded = true;
   }

   unsigned long e ;
   int flags, line;
   const char *data, *file;
   e = ERR_get_error_line_data( &file, &line, &data, &flags );
   if (e == 0){
      //No more crypto error, just return false.
      return false;
   }

   ErrorDesc.append("[Crypt Error]: ");

   char ErrBuf[StringBufSize];
   ERR_error_string_n(e, ErrBuf, StringBufSize);
   //long len = strlen(ErrBuf);
   ErrorDesc.append( ErrBuf, strlen(ErrBuf) );
   if ( flags & ERR_TXT_STRING ){
      //The OpenSSL flag ERR_TXT_STRING is used
      //when the data field is a text string we can
      //safely handle. However, even in this case we 
      //will impose a maximum length, just in case.
      if ( strlen(data) < StringBufSize ){
         ErrorDesc.append(" :data=");
         ErrorDesc.append(data);
      }
   }
   return true;
}

// this function is a C++ trick to get the correct operator
// called in each case. All this does is call 'output', a member 
// function that can be overridden in derived classes. This is
// necessary since we are using friend functions (and the binding
// to friend functions is not polymorphic; the static type is 
// always used). This indirection allows a caller to write:
//
// try {
//    DoWorkThatThrowsTheseExceptions();
// } catch ( ve_base& except ){
//    cerr << except << endl;
// }
// 
// and have the correct friend operator get called. If we didn't do
// this then the static type (ve_base) would always get called, whichever
// type of exception was actually thrown. We want to use the friend
// functions rather than member operators since this allows the natural 
// semantics of having the stream on the left hand side of the expression;
// if we used a member function, the exception instance would have to be
// there and it would look odd.
//
ostream& operator<<(ostream &s, ve_base &vb) {
   return vb.output(s);
}

ostream& operator<<(ostream &s, ve_badsig &vb) {
   s << "[BADSIG]: " << vb.m_Error << endl;
   return s;
}

// And the following are the actual friend operators that get called from
// each of the derived classes and output their data correctly.
ostream& operator<<(ostream &s, ve_file &vf) {
   s << "[BADFILE]: " << vf.m_Error << ": '" << vf.m_Filename << "'";
   return s;
}

// output an instance of the ve_crypt exception class onto
// a stream. This involves popping all of the errors and
// sending each one. Each is placed onto a newline.
//
ostream& operator<<(ostream &s, ve_crypt &vc) {
   string ErrorDesc;
   s << "[VE_CRYPT] " << vc.m_Message << endl;
   while ( vc.getNextCryptErr( ErrorDesc ) ){
      s << ErrorDesc << endl;
      ErrorDesc.clear();
   }
   return s;
}

// output an instance of the ve_badcert exception class onto
// a stream. This involves popping all of the errors and
// sending each one. Each is placed onto a newline.
//
ostream& operator<<(ostream &s, ve_badcert &vb) {
   s << "[VE_BADCERT]: " << vb.m_Error << endl;
   for (
      CertificateTracker::iter_type iter = CertificateTracker::GetInstance().GetProblems();
      iter != CertificateTracker::GetInstance().GetEnd();
      iter++
   ){
      s << *iter << endl;
   }
   CertificateTracker::GetInstance().Clear();
   return s;
}

// Output an instance of the ve_logic exception class onto
// a stream.
ostream& operator<<(ostream &s, ve_logic &vl) {
   s << "[LOGIC]: " << vl.m_Error;
   return s;
}

} //namespace verify_exceptions

