//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../settings.h"

GrooveThread::GrooveThread(): juce::Thread("Groove_Thread")
{

}

/*GrooveThread::~GrooveThread()
{
    this->stopThread(2000);
}*/

// No need to override in the children classes
void GrooveThread::run()
{
    bool bExit = threadShouldExit();
    while (!bExit)
    {
        //QueueDataProcessor();
        // DBG("GROOVETHREAD RUNNING"); // FIXME maybe need a lock to avoid crash

        bExit = threadShouldExit();
        sleep (thread_settings::GrooveThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
    }
}

void GrooveThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
}