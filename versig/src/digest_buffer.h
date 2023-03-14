// digest_buffer.h : interface for the digest_file_buffer class.
//
//  20030902 Original code from DC version 1.0.0
//
//////////////////////////////////////////////////////////////////////

#if !defined(_DIGEST_FILE_BUFFER_H_INCLUDED_)
#    define _DIGEST_FILE_BUFFER_H_INCLUDED_

#include "hash.h"
#include "signature.h"

# include <iostream>
#    include <list>

namespace manifest
{
    class digest_file_buffer
    {
    private:
        unsigned long file_buf_max_; // a maximum size to read into the _file_buf, to stop DOS attacks (default 128k)
        std::string file_buf_;       // buffer containing the bit of the file over which the signature is calculated
        std::vector<signature> signatures_; // the digital signatures at the end of the file
    public:
        digest_file_buffer() : file_buf_max_(1024 * 128) {}
//        void set_file_body_limit(unsigned long lim) { file_buf_max_ = lim; }

        [[nodiscard]] const std::string&     file_body()  const { return file_buf_; }
        [[nodiscard]] std::vector<signature> signatures() const { return signatures_; }

        friend std::istream& operator>>(std::istream &s, digest_file_buffer &v);
    };
    std::istream& operator>>(std::istream &s, digest_file_buffer &v);
}

namespace VerificationTool
{
    using namespace std;

    class digest_file_buffer : public manifest::digest_file_buffer
    {
    };

} // namespace VerificationTool

// re-enable warning about very long type identifiers in debug info
//#pragma warning (default : 4786)

#endif // !defined(_DIGEST_FILE_BUFFER_H_INCLUDED_)
