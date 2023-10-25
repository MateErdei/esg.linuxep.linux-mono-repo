#include <signal.h>
#include <unistd.h>

void s_signal_handler(int /*s*/)
{
    return;
}

int main(void) {
    struct sigaction action {};
    action.sa_handler = s_signal_handler;
    action.sa_flags = SA_RESTART;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, nullptr);

    while (true)
    {
        sleep(10);
    }
}