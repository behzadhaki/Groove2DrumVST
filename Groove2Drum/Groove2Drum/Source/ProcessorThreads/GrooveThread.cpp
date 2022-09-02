//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../Includes/UtilityMethods.h"


using namespace torch::indexing;


// ============================================================================================================
// ===          Preparing Thread for Running
// ============================================================================================================
// ------------------------------------------------------------------------------------------------------------
// ---         Step 1 . Construct
// ------------------------------------------------------------------------------------------------------------
GrooveThread::GrooveThread():
    juce::Thread("Groove_Thread")
{

    ProcessBlockToGrooveThreadQue = nullptr;
    GrooveThreadToModelThreadQue = nullptr;
    GrooveThread2GGroovePianoRollWidgetQue = nullptr;
    GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = nullptr;
    APVTS2GrooveThread_groove_vel_offset_ranges_Que = nullptr;

    readyToStop = false;

    monotonic_groove = MonotonicGroove<HVO_params::time_steps>();
    
}
// ------------------------------------------------------------------------------------------------------------
// ---         Step 2 . give access to resources needed to communicate with other threads
// ------------------------------------------------------------------------------------------------------------
void GrooveThread::startThreadUsingProvidedResources(LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>* ProcessBlockToGrooveThreadQuePntr,
                                                     MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQuePntr,
                                                     MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* GrooveThread2GGroovePianoRollWidgetQuesPntr,
                                                     LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr,
                                                     LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    ProcessBlockToGrooveThreadQue = ProcessBlockToGrooveThreadQuePntr;
    GrooveThreadToModelThreadQue = GrooveThreadToModelThreadQuePntr;
    GrooveThread2GGroovePianoRollWidgetQue = GrooveThread2GGroovePianoRollWidgetQuesPntr;
    GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr;
    APVTS2GrooveThread_groove_vel_offset_ranges_Que = APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr;
    startThread();

}
// ------------------------------------------------------------------------------------------------------------
// ---         Step 3 . start run() thread by calling startThread().
// ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
// ---                  (Implement what the thread does inside the run() method
// ------------------------------------------------------------------------------------------------------------
void GrooveThread::run()
{
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    // flag to check if groove has been updated in an iteration
    bool isNewGrooveAvailable;

    // flag to check if new HandDrawn Note has been added to groove
    bool isNewGrooveAvailableUsingHandDrawnNote;


    // local variables to keep track of vel_range = (min_vel, max_vel) and offset ranges
    vel_range = {HVO_params::_min_vel, HVO_params::_max_vel};
    offset_range = {HVO_params::_min_offset, HVO_params::_max_offset};


    while (!bExit)
    {
        // only need to recalc if new info received in queues
        isNewGrooveAvailable = false;
        isNewGrooveAvailableUsingHandDrawnNote = false;

        if (shouldResetGroove)
        {
            monotonic_groove.resetGroove();
            shouldResetGroove = false;
            isNewGrooveAvailable = true;
        }

        if (ProcessBlockToGrooveThreadQue != nullptr)
        {

            // see if new BasicNotes received from gui by hand drawing dragging a note
            while (GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue->getNumReady() > 0)
            {
                // Step 1. get new note
                auto handDrawnNote = GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue->pop(); // here cnt result is 3

                {
                    // step 2. add to groove
                    bool grooveUpdated = monotonic_groove.overdubWithNote(handDrawnNote, true);

                    // activate sending flag if at least one note added
                    if (grooveUpdated)
                    {
                        isNewGrooveAvailableUsingHandDrawnNote = true;
                    }
                }
            }

            // see if new BasicNotes received from main processblock
            BasicNote read_note;
            while (ProcessBlockToGrooveThreadQue->getNumReady() > 0 and not this->threadShouldExit())
            {

                // Step 1. get new note
                ProcessBlockToGrooveThreadQue->ReadFrom(&read_note, 1); // here cnt result is 3

                // groove should only be updated in playback mode
                //if (read_note.capturedInPlaying) // todo uncomment
                {
                    // step 2. add to groove
                    bool grooveUpdated = monotonic_groove.overdubWithNote(read_note);

                    // activate sending flag if at least one note added
                    if (grooveUpdated)
                    {
                        isNewGrooveAvailable = true;
                    }
                }
            }

            // apply compression if new notes overdubbed
            if (isNewGrooveAvailable or isNewGrooveAvailableUsingHandDrawnNote)
            {
                monotonic_groove.hvo.compressAll();
            }
        }

        // see if new control params received from the gui
        if (APVTS2GrooveThread_groove_vel_offset_ranges_Que != nullptr)
        {
            array<float, 4> newVelOffsetrange {};

            while (APVTS2GrooveThread_groove_vel_offset_ranges_Que->getNumReady() > 0
                   and not this->threadShouldExit())
            {
                // Step 1. get new vel/offset ranges received
                APVTS2GrooveThread_groove_vel_offset_ranges_Que->ReadFrom(&newVelOffsetrange, 1);

                // update local range values
                vel_range = {newVelOffsetrange[0], newVelOffsetrange[1]};
                offset_range = {newVelOffsetrange[2], newVelOffsetrange[3]};

                // update groove with the new ranges
                monotonic_groove.hvo.updateCompressionRanges(newVelOffsetrange, true);

                // activate sending flag
                isNewGrooveAvailable = true;
            }
        }


        // Send groove to other threads if new one available
        if (isNewGrooveAvailable or isNewGrooveAvailableUsingHandDrawnNote)
        {
            if (GrooveThreadToModelThreadQue != nullptr)
            {
                // send to Model Thread to pass through the model
                GrooveThreadToModelThreadQue->push(monotonic_groove);
            }
            if (GrooveThread2GGroovePianoRollWidgetQue != nullptr)
            {
                // send groove to be displayed on the interface
                GrooveThread2GGroovePianoRollWidgetQue->push(monotonic_groove);
            }

        }

        bExit = threadShouldExit();
        // sleep (thread_settings::GrooveThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
        // sleep(1);
    }

}

// ============================================================================================================
// ===          Preparing Thread for Stopping
// ============================================================================================================

void GrooveThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
    readyToStop = true;
}



GrooveThread::~GrooveThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }

}

void GrooveThread::ForceResetGroove()
{
    shouldResetGroove = true;
}







