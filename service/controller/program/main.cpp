#include "Controller.h"

#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <unistd.h>

using namespace OIC;
using namespace Service;

Controller::Ptr controller;


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
    controller = Controller::Ptr(Controller::getInstance());
    controller->start();
    signal(SIGINT, handleSigInt);
    while (!g_quitFlag)
    {
        if(OCProcess() != OC_STACK_OK)
        {
            controller->stop();
            std::cout << "OCStack process error" << std::endl;
            return 0;
        }
        sleep(1);
    }

    controller->stop();

	return 0;
}

