//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../Includes/UtilityMethods.h"


using namespace torch::indexing;




GrooveThread::GrooveThread():
    juce::Thread("Groove_Thread")
{
    incomingNoteQue = nullptr;
    VelScaleParamQue = nullptr;
    scaledGrooveQue = nullptr;
    text_message_queue_for_debugging = nullptr;

    readyToStop = false;

    monotonic_groove = MonotonicGroove<time_steps>();

    gridlines = torch::range(0, 7.9, 0.25);

}

void GrooveThread::start_Thread(
    LockFreeQueue<Note, settings::note_queue_size>* incomingNoteQuePntr,
    LockFreeQueue<torch::Tensor, settings::torch_tensor_queue_size>* scaledGrooveQuePntr,
    LockFreeQueue<float, settings::control_params_queue_size>* VelScaleParamQuePntr,
    StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue_for_debuggingPntr
)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    incomingNoteQue = incomingNoteQuePntr;
    VelScaleParamQue = VelScaleParamQuePntr;
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

void GrooveThread::run()
{
    bool bExit = threadShouldExit();
    bool shouldReCalc;

    Note read_note;

    /*
    HVO<settings::time_steps, 2> groove_test;
    groove_test.randomize();
    vector<Note> notes_from_hvo = groove_test.getModifiedNotes();

    showMessageinEditor(
        text_message_queue_for_debugging, groove_test.getStringDescription(false),
        "unscaled groove", false);

    DBG("Unscaled offsets");
    DBG(groove_test.getStringDescription(false));
    groove_test.compressOffsets(0, 0, 1);
    groove_test.compressVelocities(0, 2, 4);
    DBG("Scaled offsets");
    DBG(groove_test.getStringDescription(true));*/


    /*DBG(torch2string(groove_unscaled.offsets));

    for (int i=0; i<notes_from_hvo.size();i++)
    {
        DBG(notes_from_hvo[i].getStringDescription());
    }
    */
    float VelScaleParam = 1;

    while (!bExit)
    {
        shouldReCalc = false;

        if (incomingNoteQue != nullptr)
        {
            while (incomingNoteQue->getNumReady() > 0 and not this->threadShouldExit())
            {
                // Step 1. get new note
                incomingNoteQue->ReadFrom(&read_note, 1); // here cnt result is 3

                // step 2. add to groove
                monotonic_groove.ovrerdubWithNote(read_note);

                shouldReCalc = true;
            }
        }

        /*
        if (incomingNoteQue != nullptr)
        {
            while ()
            {
             // check parameters in queues here
             shouldReCalc = true;
            }
        }
        */

        if (shouldReCalc)
        {
            showMessageinEditor(
                text_message_queue_for_debugging, monotonic_groove.getStringDescription(false),
                "unscaled groove", false);

            // modify groove if needed
            monotonic_groove.hvo.compressOffsets(0, -0.1, 0.1);
            monotonic_groove.hvo.compressVelocities(0, .2, 2);

            showMessageinEditor(
                text_message_queue_for_debugging, monotonic_groove.getStringDescription(true),
                "scaled groove", false);
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


void GrooveThread::Send()
{
    // scaledGrooveQue->WriteTo(&unscaled_groove_overdubbed, 1);
}




