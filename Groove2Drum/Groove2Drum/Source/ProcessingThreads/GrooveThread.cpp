//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../Includes/UtilityMethods.h"


using namespace torch::indexing;




GrooveThread::GrooveThread():
    juce::Thread("Groove_Thread")
{
    note_toProcess_que = nullptr;
    veloff_fromGui_que = nullptr;
    groove_toProcess_que = nullptr;
    text_toGui_que_for_debugging = nullptr;
    groove_toGui_que = nullptr;
    readyToStop = false;



    monotonic_groove = MonotonicGroove<time_steps>();

    gridlines = torch::range(0, 7.9, 0.25);

}

void GrooveThread::start_Thread(
    LockFreeQueue<Note, settings::processor_io_queue_size>* note_toProcess_quePntr,
    MonotonicGrooveQueue<settings::time_steps, processor_io_queue_size>* groove_toProcess_quePntr,
    LockFreeQueue<array<float, 4>, gui_io_queue_size>* veloff_fromGui_quePntr,
    MonotonicGrooveQueue<settings::time_steps, gui_io_queue_size>* groove_toGui_quePntr,
    StringLockFreeQueue<settings::gui_io_queue_size>* text_toGui_que_for_debuggingPntr
)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    note_toProcess_que = note_toProcess_quePntr;
    veloff_fromGui_que = veloff_fromGui_quePntr;
    groove_toProcess_que = groove_toProcess_quePntr;
    groove_toGui_que = groove_toGui_quePntr;
    text_toGui_que_for_debugging = text_toGui_que_for_debuggingPntr;

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
        if (note_toProcess_que != nullptr)
        {
            Note read_note;

            while (note_toProcess_que->getNumReady() > 0 and not this->threadShouldExit())
            {
                // Step 1. get new note
                note_toProcess_que->ReadFrom(&read_note, 1); // here cnt result is 3

                // step 2. add to groove
                monotonic_groove.ovrerdubWithNote(read_note);

                // activate recalculation flag
                isNewGrooveAvailable = true;
            }
        }

        // see if new control params received from the gui
        if (veloff_fromGui_que != nullptr)
        {
            array<float, 4> newVelOffsetrange {};

            while (veloff_fromGui_que->getNumReady() > 0
                   and not this->threadShouldExit())
            {
                // Step 1. get new vel/offset ranges received
                veloff_fromGui_que->ReadFrom(&newVelOffsetrange, 1);

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
            if (groove_toProcess_que != nullptr)
            {
                groove_toProcess_que->push(monotonic_groove);
            }

            // send groove to be displayed on the interface
            if (groove_toGui_que != nullptr)
            {
                groove_toGui_que->push(monotonic_groove);
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
    // groove_toProcess_que->WriteTo(&unscaled_groove_overdubbed, 1);
}




