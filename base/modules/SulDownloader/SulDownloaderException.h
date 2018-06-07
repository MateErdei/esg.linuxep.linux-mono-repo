//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_SULDOWNLOADEREXCEPTIONS_H
#define EVEREST_SULDOWNLOADEREXCEPTIONS_H
#include <exception>
#include <string>
namespace SulDownloader
{
    class SulDownloaderException : public std::exception
    {
    public:
        SulDownloaderException( std::string  message);
        const char * what() const noexcept override ;
    private:
        std::string m_message;
    };


}
#endif //EVEREST_SULDOWNLOADEREXCEPTIONS_H
