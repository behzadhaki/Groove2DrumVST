//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../settings.h"
#include "../Includes/UtilityMethods.h"
#include "../Includes/Representations.h"

using namespace torch::indexing;


GrooveThread::GrooveThread():
    juce::Thread("Groove_Thread")
{
    readyToStop = false;

    groove_overdubbed = torch::zeros({settings::time_steps, 3});
    onset_ppqs_in_groove = torch::zeros({settings::time_steps, 1});

    gridlines = torch::range(0, 7.9, 0.25);

}

void GrooveThread::start_Thread(
    LockFreeQueue<Note, settings::note_queue_size>* incomingNoteQuePntr,
    LockFreeQueue<torch::Tensor, settings::torch_tensor_queue_size>* scaledGrooveQuePntr,
    float* VelScaleParamPntr,
    StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue_for_debuggingPntr
)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    incomingNoteQue = incomingNoteQuePntr;
    VelScaleParam = VelScaleParamPntr;
    scaledGrooveQue = scaledGrooveQuePntr;

    text_message_queue_for_debugging = text_message_queue_for_debuggingPntr;

    startThread();

}

GrooveThread::~GrooveThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }
}

void GrooveThread::reset_groove()
{
    groove_overdubbed = torch::zeros({settings::time_steps, 3});
}

void GrooveThread::run()
{
    bool bExit = threadShouldExit();

    Note read_note;

    while (!bExit)
    {
        if (incomingNoteQue != nullptr)
        {
            while (incomingNoteQue->getNumReady() > 0 and not this->threadShouldExit())
            {
                // Step 1. Convert Note to GrooveEvent
                //      (i.e. get rid of pitch info, and calculate grid_index and
                //      offset)
                incomingNoteQue->ReadFrom(&read_note, 1); // here cnt result is 3
                GrooveEvent groove_event(read_note);
                showMessageinEditor(
                    text_message_queue_for_debugging, groove_event.getStringDescription(),
                    "groove_event in groove_thread", false);



                /*NoteProcessor(read_note);
                GrooveScaler();
                Send();*/
            }
        }

        bExit = threadShouldExit();
        sleep (thread_settings::GrooveThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
    }
}

void GrooveThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
    readyToStop = true;
}


void GrooveThread::NoteProcessor(Note latest_Note)
{
    // auto pitch = latest_Note.note;
    // auto ppq = latest_Note.time.ppq;
    // auto vel = latest_Note.velocity;
}

void GrooveThread::GrooveScaler()
{
    groove_overdubbed = groove_overdubbed * (*VelScaleParam);
}

void GrooveThread::Send()
{
    scaledGrooveQue->WriteTo(&groove_overdubbed, 1);
}




