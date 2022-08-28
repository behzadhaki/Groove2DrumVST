//
// Created by Behzad Haki on 2022-08-05.
//

#include "GrooveThread.h"
#include "../Includes/UtilityMethods.h"


using namespace torch::indexing;




GrooveThread::GrooveThread():
    juce::Thread("Groove_Thread")
{

    ProcessBlockToGrooveThreadQues = nullptr;
    GrooveThreadToModelThreadQues = nullptr;
    GrooveThread2GGroovePianoRollWidgetQues = nullptr;
    GroovePianoRollWidget2GrooveThreadQues = nullptr;

    readyToStop = false;

    monotonic_groove = MonotonicGroove<HVO_params::time_steps>();
    
}

void GrooveThread::startThreadUsingProvidedResources(IntraProcessorFifos::ProcessBlockToGrooveThreadQues* ProcessBlockToGrooveThreadQuesPntr,
                                                     IntraProcessorFifos::GrooveThreadToModelThreadQues* GrooveThreadToModelThreadQuesPntr,
                                                     GuiIOFifos::GrooveThread2GGroovePianoRollWidgetQues* GrooveThread2GGroovePianoRollWidgetQuesPntr,
                                                     GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQuesPntr)
{
    // get the pointer to queues and control parameters instantiated
    // in the main processor thread
    ProcessBlockToGrooveThreadQues = ProcessBlockToGrooveThreadQuesPntr;
    GrooveThreadToModelThreadQues = GrooveThreadToModelThreadQuesPntr;
    GrooveThread2GGroovePianoRollWidgetQues = GrooveThread2GGroovePianoRollWidgetQuesPntr;
    GroovePianoRollWidget2GrooveThreadQues = GroovePianoRollWidget2GrooveThreadQuesPntr;
    startThread();

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
    DBG("RESETTING GROOVE IN GROOVE THREAD _-> to be implemented");
    shouldResetGroove = true;
}

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

        if (ProcessBlockToGrooveThreadQues != nullptr)
        {

            // see if new BasicNotes received from gui by hand drawing dragging a note
            while (GroovePianoRollWidget2GrooveThreadQues->manually_drawn_notes.getNumReady() > 0)
            {
                // Step 1. get new note
                auto handDrawnNote = GroovePianoRollWidget2GrooveThreadQues->manually_drawn_notes.pop(); // here cnt result is 3

                // groove should only be updated in playback mode
                //if (read_note.capturedInPlaying) // todo uncomment
                {
                    // step 2. add to groove
                    bool grooveUpdated = monotonic_groove.overdubWithNote(handDrawnNote);

                    // activate sending flag if at least one note added
                    if (grooveUpdated)
                    {
                        isNewGrooveAvailableUsingHandDrawnNote = true;
                    }
                }
            }

            // see if new BasicNotes received from main processblock
            BasicNote read_note;
            while (ProcessBlockToGrooveThreadQues->new_notes.getNumReady() > 0 and not this->threadShouldExit())
            {

                // Step 1. get new note
                ProcessBlockToGrooveThreadQues->new_notes.ReadFrom(&read_note, 1); // here cnt result is 3

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
            if (isNewGrooveAvailable)
            {
                monotonic_groove.hvo.compressAll();
            }
        }
        
        // see if new control params received from the gui
        if (GroovePianoRollWidget2GrooveThreadQues != nullptr)
        {
            array<float, 4> newVelOffsetrange {};

            while (GroovePianoRollWidget2GrooveThreadQues->newVelOffRanges.getNumReady() > 0
                   and not this->threadShouldExit())
            {
                // Step 1. get new vel/offset ranges received
                GroovePianoRollWidget2GrooveThreadQues->newVelOffRanges.ReadFrom(&newVelOffsetrange, 1);

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
            if (GrooveThreadToModelThreadQues != nullptr)
            {
                // send to Model Thread to pass through the model
                GrooveThreadToModelThreadQues->new_grooves.push(monotonic_groove);
            }
            if (isNewGrooveAvailable)
            {
                // send groove to be displayed on the interface
                GrooveThread2GGroovePianoRollWidgetQues->new_grooves.push(monotonic_groove);
            }

        }

        bExit = threadShouldExit();
        // sleep (thread_settings::GrooveThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
        // sleep(1);
    }

}

void GrooveThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
    readyToStop = true;
}






