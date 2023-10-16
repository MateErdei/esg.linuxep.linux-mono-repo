// Copyright 2003-2023 Sophos Limited. All rights reserved.

// verify_exceptions.h. This header file defines exception classes to
// be thrown in response to error events while reading and verifying.
//
//  20030902 RW version 1.0.0
//
// Exception classes:
//    All of the exceptions derive from ve_base, so all offer the
//    function GetErrorCode. Use this to get back the status code
//    from the SignedFile::status_enum enumeration. These codes should
//    be converted into translatable strings since they report
//    the fundamental status of the operation.
//    In addition, all of the exception classes can be output to
//    a stream (such as cerr) for debugging purposes. These
//    messages are always in English so are not good to show to a user.
//
//    ve_crypt is thrown whenever an OpenSSL error is detected. This
//    does not happen for an 'expected' crypto event like a verify
//    failing or a certificate being revoked. These are reported
//    differently. If a ve_crypt exception has been thrown, OpenSSL
//    has failed; maybe a corrupted file or a real sytem problem.
//
//    ve_badsig is thrown if the signature verify itself fails. There
//    is little extra information available from this. Could mean that
//    the data has been corrupted or tampered with or that the certificate
//    that was used to attempt the verification is not the correct one.
//
//    ve_badcert is thrown if the signature itself was fine, but that the
//    certificate used to verify it could not itself be verified. This could
//    mean a revoked certificate, an expired certificate or that the cert path
//    did not have a trust path to a trusted root certificate. The names of the
//    failed certs are available from the GetBadCertNames() function since this
//    is useful to diagnose where the problem lies.
//
//    ve_file is thrown if a file cannot be found or has the wrong format.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_VERIFY_EXCEPTIONS_H_INCLUDED_)
#    define _VERIFY_EXCEPTIONS_H_INCLUDED_

#    include "CertificateTracker.h"
#    include "SophosCppStandard.h"
#    include "signed_file.h"

#    include <string>
#    include <vector>

// This next define is needed as a macro since it gets used to
// size arrays. I'd prefer to have it as a static const, but
// then it wouldn't work. This constant allows us to set a
// maximum string length when we create temp char buffers
// for use with the OpenSSL functions and copy into them.
#    define StringBufSize 512

namespace verify_exceptions
{
    using namespace std;
    using namespace VerificationTool;

    // All exceptions thrown will derive from this base class.
    // Note that the error code stored in here is one of the
    // SignedFile::status_enum codes and these should be used to
    // produce a suitable, translated message to the end user.
    //
    class ve_base : public std::runtime_error
    {
    protected:
        SignedFile::status_enum m_Error;

    public:
        // Constructor from an error code
        ve_base(const std::string& message, const SignedFile::status_enum ErrorCode) :
            std::runtime_error(message),
            m_Error(ErrorCode)
        {
        }

        explicit ve_base(const SignedFile::status_enum ErrorCode) :
            ve_base("Verification exception", ErrorCode)
        {
        }

#    if CPPSTD == 11
        ~ve_base() override = default;
#    else
        virtual ~ve_base() NOEXCEPT
        {
        }
#    endif

        // Copy constructor (defined for the use of derived classes)
        //      ve_base( const ve_base& rhs ){ m_Error = rhs.m_Error; }

        // Assignment operator (defined for the use of derived classes)
        // ve_base& operator = (const ve_base& rhs){
        //   m_Error = rhs.m_Error;
        //   return *this;
        //}

        // This function allows the error code to be returned.
        [[nodiscard]] SignedFile::status_enum getErrorCode() const { return m_Error; }

        // This function ensures that the friend operator correctly redirects to
        // a derived class if accessed through a base class reference.
        // This is pure virtual to ensure that no instance of this class is EVER
        // created (if it were, the act of writing it to an ostream would be
        // infinitely recursive!).
        // virtual ostream& output(ostream &s) = 0;

        // Output the class to a stream.
        //      friend ostream& operator<<(ostream &s, ve_base &vb);
    };

    // This class derives from ve_base. It represents any error
    // caused as a result of file manipulation (opening a file
    // that doesn't exist or to which suitable permissions have
    // not been granted, for example
    class ve_file : public ve_base
    {
    protected:
        string m_Filename;

    public:
        // Constructor from an error code and filename
        explicit ve_file(
            const SignedFile::status_enum ErrorCode,
#    if CPPSTD == 11
            string Filename
#    else
            const string& Filename
#    endif
            ) :
            ve_base(ErrorCode),
            m_Filename(STDMOVE(Filename))
        {
        }

#    if CPPSTD != 11
        virtual ~ve_file() NOEXCEPT
        {
        }
#    endif

        // Copy constructor
        //      ve_file( const ve_file& rhs ) : ve_base(rhs.m_Error), m_Filename(rhs.m_Filename)
        //      {}

        // Assignment operator
        // ve_file& operator = ( const ve_file& rhs ) {
        //   ve_base::operator=(rhs);
        //   m_Filename = rhs.m_Filename;
        //   return *this;
        //}

        // Allow the filename to be accessed
        //      const string& GetFilename() const { return m_Filename; }

        // Overload of the base class function to ensure that
        // the correct operator gets called.
        // virtual ostream& output(ostream &s) { s << *this; return s; }

        // Define this as a friend since this allows a natural syntax
        // to be used to call the operator.
        friend ostream& operator<<(ostream& s, ve_file& vf);
    };

    // This class derives from ve_base. It represents any exception
    // which is caused by an OpenSSL error. This class encompasses
    // the more detailed information from OpenSSL itself. This is
    // available as a set of error strings that OpenSSL provides. These are
    // useful for debugging, but are NOT suitable to be
    // exposed to users for 2 reasons:
    //  1) They are distinctly non-user-friendly
    //  2) They are only available in English.
    //
    // So while this class offers the ability to retrieve the extra
    // OpenSSL information, it is only really suitable for debug
    // or tech support use and shouldn't be written into a user-
    // readable log file.
    //
    class ve_crypt : public ve_base
    {
        string m_Message;

    public:
        explicit ve_crypt(STRARG Msg) : ve_base(SignedFile::openssl_error), m_Message(STDMOVE(Msg))
        {
        }
        //      ve_crypt( const ve_crypt& rhs ) : ve_base(rhs), m_Message(rhs.m_Message) {}
        // ve_crypt& operator=(const ve_crypt& rhs) {
        //   ve_base::operator=(rhs);
        //   m_Message = rhs.m_Message;
        //   return *this;
        //}

        [[nodiscard]] std::string message() const
        {
            return m_Message;
        }

#    if CPPSTD != 11
        virtual ~ve_crypt() NOEXCEPT
        {
        }
#    endif

        // OpenSSL errors are in an error queue from which
        // we need to pop them. Call this function until it
        // returns false. Each true return will append the
        // next crypto error to the supplied STL string.
        // These crypto errors are not translatable (the string
        // table is only in English) and are not suitable for showing
        // to a user in any event. Only tech support would have any use
        // for them, really.
        bool getNextCryptErr(string& ErrorDesc) const;

        // This function ensures that the friend operator
        // correctly redirects to a derived class if accessed
        // through a base class reference.
        // virtual ostream& output(ostream &s) { s << *this; return s; }

        friend ostream& operator<<(ostream& s, ve_crypt& vc);
    };

    // This exception class is thrown when a signature fails to
    // verify. It contains no additional information since there
    // is little else to say.
    // This class is only thrown when an apparently valid signature
    // fails to verify. It means either that the signature has been
    // altered or corrupted or that the certificate being used to
    // verify is not the correct one. It is NOT thrown if the cert
    // path cannot be verified. In that case, ve_badcert is thrown.
    //
    class ve_badsig : public ve_base
    {
    public:
        ve_badsig(const std::string& message)
            : ve_base(message, SignedFile::bad_signature)
        {}

        ve_badsig() : ve_base(SignedFile::bad_signature)
        {
        }
        // This function ensures that the friend operator
        // correctly redirects to a derived class if accessed
        // through a base class reference.
        // virtual ostream& output(ostream &s) { s << *this; return s; }

#    if CPPSTD != 11
        virtual ~ve_badsig() NOEXCEPT
        {
        }
#    endif

        friend ostream& operator<<(ostream& s, ve_badsig& vc);
    };

    class ve_missingsig : public ve_badsig
    {
    public:
        using ve_badsig::ve_badsig;
        // This function ensures that the friend operator
        // correctly redirects to a derived class if accessed
        // through a base class reference.
        // virtual ostream& output(ostream &s) { s << *this; return s; }

        friend ostream& operator<<(ostream& s, ve_missingsig& vc);
    };

    // This exception class is thrown when a certificate path cannot
    // be verified back to a trusted root. This might indicate that a
    // certificate has been revoked, or has expired or it may indicate
    // some other problem with the chain.
    //
    class ve_badcert : public ve_base
    {
        string m_BadCertNames;

    public:
        ve_badcert() :
            ve_base(SignedFile::bad_certificate),
            m_BadCertNames(CertificateTracker::GetInstance().GetNames())
        {
        }

#    if CPPSTD != 11
        virtual ~ve_badcert() NOEXCEPT
        {
        }
#    endif

        //      string GetBadCertNames() { return m_BadCertNames; }

        // This function ensures that the friend operator
        // correctly redirects to a derived class if accessed
        // through a base class reference.
        // virtual ostream& output(ostream &s) { s << *this; return s; }

        friend ostream& operator<<(ostream& s, ve_badcert& vc);
    };

    // This exception class is thrown when a logic error occurs.
    // Typically, it indicates that an operation has been attempted
    // by an object in an unfit state to perform the operation.
    // class ve_logic : public ve_base {
    // protected:
    //   string m_Filename;
    // public:
    //   explicit ve_logic(
    //      const SignedFile::status_enum ErrorCode
    //   ) : ve_base(ErrorCode) {}

    //   ve_logic( const ve_logic& rhs ) : ve_base(rhs.m_Error) {}

    //   //ve_logic& operator = ( const ve_logic& rhs ) {
    //   //   ve_base::operator=(rhs);
    //   //   return *this;
    //   //}

    //   //virtual ostream& output(ostream &s) { s << *this; return s; }

    //   friend ostream& operator<<(ostream &s, ve_logic &vl);

    //   virtual ~ve_logic() {}
    //};

} // namespace verify_exceptions
#endif
