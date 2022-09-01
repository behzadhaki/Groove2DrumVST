//
// Created by Behzad Haki on 2022-08-05.
//

#ifndef JUCECMAKEREPO_GROOVETHREAD_H
#define JUCECMAKEREPO_GROOVETHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../InterThreadFifos.h"
#include "../settings.h"

class GrooveThread:public juce::Thread, public juce::ChangeBroadcaster
{
public:
    // constructor
    GrooveThread();

    // destructor
    ~GrooveThread() override;

    // give access to resources needed to communicate with other threads
    void startThreadUsingProvidedResources(LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>* ProcessBlockToGrooveThreadQuePntr,
                                           MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQuePntr,
                                           MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* GrooveThread2GGroovePianoRollWidgetQuesPntr,
                                           GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQuesPntr);

    // run this in destructor destructing object
    void prepareToStop();

    // don't call this from the parent thread
    // instead, use startThread() from which
    // run() is explicitly called
    void run() override;

    // reset Groove if requested
    void ForceResetGroove();

    // Used to check if thread is ready to be stopped
    // used to check if a parent thread has externally
    // requested the thread to stop
    bool readyToStop;

private:

    LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>* ProcessBlockToGrooveThreadQue;
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQue;


    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* GrooveThread2GGroovePianoRollWidgetQue;
    GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues* GroovePianoRollWidget2GrooveThreadQues;

    //---- Control Parameters from GUI -------------------------------------
    // LockFreeQueue<array<float, 4>, GeneralSettings::gui_io_queue_size>* veloff_fromGui_que{};
    array<float, 2> vel_range;
    array<float, 2> offset_range;

    // ---- sends data to gui -----------------------------------------------
    // the queue for sending the updated groove to the next thread
    // MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* groove_toGui_que{};


    // ;
    //----------------------------------------------------------------------

    // ---- Locally Used for calculations ----------------------------------

    // an internal HVO instance of size {time_steps, 1 voice)
    // for tracking the unscaled groove
    MonotonicGroove<HVO_params::time_steps> monotonic_groove;

    // torch::Tensor unscaled_groove_overdubbed;

    // an internal tensor of size { time_steps , 1}
    // for tracking the actual onset time of registered
    // notes in the groove_overdubbed tensor
    // torch::Tensor onset_ppqs_in_groove;


    // int find_nearest_gridline(float ppq);
    // bool shouldReplace(BasicNote latest_Note);

    // Flag to see if reseting the groove has been requested
    bool shouldResetGroove {false};
    //----------------------------------------------------------------------

};

#endif //JUCECMAKEREPO_GROOVETHREAD_H
