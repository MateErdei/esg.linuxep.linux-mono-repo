#include <signal.h>
#include <unistd.h>

void s_signal_handler(int /*s*/)
{
    return;
}
int main(void) {
    struct sigaction action; // NOLINT
    action.sa_handler = s_signal_handler;
    sigaction(SIGTERM, &action, nullptr);

    while (true) {

        sleep(10);
    }
}