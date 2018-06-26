//
// Created by pair on 25/06/18.
//

#ifndef EVEREST_BASE_PIPEFORTESTS_H
#define EVEREST_BASE_PIPEFORTESTS_H
#include <string>

class PipeForTests
{
public :
    PipeForTests();
    ~PipeForTests();

    void write( const std::string  & );

    int readFd();
private:
    int m_readFd;
    int m_writeFd;
};

#endif //EVEREST_BASE_PIPEFORTESTS_H
