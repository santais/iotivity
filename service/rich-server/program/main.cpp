#include "Controller.h"
#include <signal.h>
#include <unistd.h>

using namespace OIC;
using namespace Service;
using namespace RichServer;

Controller::Ptr controller (new Controller());


/*
* This is a signal handling function for SIGINT(CTRL+C).
* A Resource Coordinator handle the SIGINT signal for safe exit.
*
* @param[in] signal
*                 signal number of caught signal.
*/
int g_quitFlag = 0;

void handleSigInt(int signum)
{
    if (signum == SIGINT)
    {
        g_quitFlag = 1;
    }
}

int main()
{
    std::cout << "Starting test program" << std::endl;
    controller->start();

    signal(SIGINT, handleSigInt);
    while (!g_quitFlag)
    {
        sleep(2);
    }

    controller->stop();

	return 0;
}

