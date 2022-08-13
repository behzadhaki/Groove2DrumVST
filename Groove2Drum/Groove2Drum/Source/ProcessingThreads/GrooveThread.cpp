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
    veloffsetScaleParamQue = nullptr;
    scaledGrooveQue = nullptr;
    text_message_queue_for_debugging = nullptr;
    grooveDisplyQue = nullptr;
    readyToStop = false;



    monotonic_groove = MonotonicGroove<time_steps>();

    gridlines = torch::range(0, 7.9, 0.25);

}

void GrooveThread::start_Thread(
    LockFreeQueue<Note, settings::note_queue_size>* incomingNoteQuePntr,
    MonotonicGrooveQueue<settings::time_steps, control_params_queue_size>* scaledGrooveQuePntr,
    LockFreeQueue<array<float, 4>, control_params_queue_size>* veloffsetScaleParamQuePntr,
    MonotonicGrooveQueue<settings::time_steps, control_params_queue_size>* grooveDisplyQuePntr,
    StringLockFreeQueue<settings::text_message_queue_size>* text_message_queue_for_debuggingPntr
)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    incomingNoteQue = incomingNoteQuePntr;
    veloffsetScaleParamQue = veloffsetScaleParamQuePntr;
    scaledGrooveQue = scaledGrooveQuePntr;
    grooveDisplyQue = grooveDisplyQuePntr;
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
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    // flag to check if groove has been updated in an iteration
    bool isNewGrooveAvailable;

    // local variables to keep track of vel_range = (min_vel, max_vel) and offset ranges
    array<float, 2> vel_range = {HVO_params::_min_vel, HVO_params::_max_vel};
    array<float, 2> offset_range = {HVO_params::_min_offset, HVO_params::_max_offset};


    while (!bExit)
    {
        // only need to recalc if new info received in queues
        isNewGrooveAvailable = false;

        // see if new notes received from main processblock
        if (incomingNoteQue != nullptr)
        {
            Note read_note;

            while (incomingNoteQue->getNumReady() > 0 and not this->threadShouldExit())
            {
                // Step 1. get new note
                incomingNoteQue->ReadFrom(&read_note, 1); // here cnt result is 3

                // step 2. add to groove
                monotonic_groove.ovrerdubWithNote(read_note);

                // activate recalculation flag
                isNewGrooveAvailable = true;
            }
        }

        // see if new control params received from the gui
        if (veloffsetScaleParamQue != nullptr)
        {
            array<float, 4> newVelOffsetrange {};

            while (veloffsetScaleParamQue->getNumReady() > 0
                   and not this->threadShouldExit())
            {
                // Step 1. get new vel/offset ranges received
                veloffsetScaleParamQue->ReadFrom(&newVelOffsetrange, 1);

                // update local range values
                vel_range[0] = newVelOffsetrange[0];
                vel_range[1] = newVelOffsetrange[1];
                offset_range[0] = newVelOffsetrange[2];
                offset_range[1] = newVelOffsetrange[3];
            }

            if( vel_range[0] != HVO_params::_min_vel or
                vel_range[1] != HVO_params::_max_vel)
            {
                monotonic_groove.hvo.compressVelocities(0, vel_range[0], vel_range[1]);
            }
            if( offset_range[0] != HVO_params::_min_offset or
                offset_range[1] != HVO_params::_max_offset)
            {
                monotonic_groove.hvo.compressOffsets(
                    0, offset_range[0], offset_range[1]);
            }

            // activate recalculation flag
            isNewGrooveAvailable = true;

        }

        // Send groove to other threads if new one available
        if (isNewGrooveAvailable)
        {
            if (scaledGrooveQue != nullptr)
            {
                scaledGrooveQue->push(monotonic_groove);
            }

            // send groove to be displayed on the interface
            if (grooveDisplyQue != nullptr)
            {
                grooveDisplyQue->push(monotonic_groove);
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


void GrooveThread::Send()
{
    // scaledGrooveQue->WriteTo(&unscaled_groove_overdubbed, 1);
}




