//
// Created by Behzad Haki on 2022-08-13.
//

#ifndef JUCECMAKEREPO_MODELTHREAD_H
#define JUCECMAKEREPO_MODELTHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/Representations.h"
#include "../Includes/LockFreeQueueTemplate.h"
#include "../settings.h"
#include "../Model/ModelAPI.h"

class ModelThread: public juce::Thread
{
public:

    // constructor
    ModelThread();

    // destructor
    ~ModelThread() override;

    // run this in destructor destructing object
    void prepareToStop();

    // give access to resources needed to communicate with other threads
    void giveAccesstoResources(
        MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>*
            groove_toProcess_quePntr,
        LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>*
            thresholds_fromGui_quePntr,
        HVOQueue<settings::time_steps, settings::num_voices,
                 settings::processor_io_queue_size>*
            HVO_toProcessforPlayback_quePntr,
        StringLockFreeQueue<settings::gui_io_queue_size>*
            text_toGui_que_for_debuggingPntr = nullptr
        );



    // don't call this from the parent thread
    // instead, use startThread() from which
    // run() is explicitly called
    void run() override;

private:
    // Used to check if thread is ready to be stopped
    // used to check if a parent thread has externally
    // requested the thread to stop
    bool readyToStop;

    // the queue for receiving notes from the GrooveThread
    MonotonicGrooveQueue<settings::time_steps,
                         processor_io_queue_size>* groove_toProcess_que{};


    MonotonicGrooveTransformerV1 modelAPI;

    LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>*  thresholds_fromGui_que;
    HVOQueue<settings::time_steps, settings::num_voices, settings::processor_io_queue_size>* HVO_toProcessforPlayback_que;
    //

    //---- Debugger -------------------------------------
    StringLockFreeQueue<settings::gui_io_queue_size>* text_toGui_que_for_debugging{};
    //----------------------------------------------------------------------



};

#endif //JUCECMAKEREPO_MODELTHREAD_H
