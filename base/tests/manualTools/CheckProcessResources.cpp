#include <Common/Process/IProcess.h>
#include <iostream>
#include <thread>
#include <unistd.h>
// This is to test that LINUXDAR-781 is fixed.
int main()
{

    int i = 0;
    while (true)
    {
        try
        {

            auto proc = Common::Process::createProcess();
            proc->exec("/bin/sleep", {"0"});
            proc->output();
            int code = 0;
            code= proc->exitCode();
            if ( code != 0)
            {
                ::abort();
            }
            if (i % 100 == 0)
            {
                std::cout << "iteration: " << i << " .last code: " << code << std::endl;
            }
            i++;

        }
        catch (std::system_error& unavailable)
        {
            if (unavailable.code().value() == EAGAIN)
            {
                std::cerr << "resource unavailable. iteration: " << i << " pid: " << ::getpid() << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(200));
                std::cerr << "try again" << std::endl;
            }
        }
    }
}