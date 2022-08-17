//
// Created by Behzad Haki on 2022-08-13.
//

#ifndef JUCECMAKEREPO_MODELTHREAD_H
#define JUCECMAKEREPO_MODELTHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructs.h"
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


    /*** give access to resources needed to communicate with other threads
     * @param groove_toProcess_quePntr
     *              (MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>)
     * @param perVoiceSamplingThresh_fromGui_quePntr
     *              (LockFreeQueue<std::array<float, settings::num_voices>,
     *              settings::gui_io_queue_size>)
     * @param HVO_toProcessforPlayback_quePntr
     *              (HVOQueue<settings::time_steps, settings::num_voices,
     *              settings::processor_io_queue_size>)
     * @param text_toGui_que_for_debuggingPntr
     *              (StringLockFreeQueue<settings::gui_io_queue_size>)
     */
    void startThreadUsingProvidedResources(
        MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>*
            groove_toProcess_quePntr,
        LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>*
            perVoiceSamplingThresh_fromGui_quePntr,
        GeneratedDataQueue<settings::time_steps, settings::num_voices, settings::processor_io_queue_size>*
            GeneratedData_toProcessforPlayback_quePntr,
        StringLockFreeQueue<settings::gui_io_queue_size>*
            text_toGui_que_for_debuggingPntr = nullptr
        );

    // don't call this from the parent thread
    // instead, use startThread from which
    // run() is implicitely called
    void run() override;

    // Used to check if thread is ready to be stopped
    // used to check if a parent thread has externally
    // requested the thread to stop
    bool readyToStop;

private:

    // the queue for receiving notes from the GrooveThread
    MonotonicGrooveQueue<settings::time_steps,
                         processor_io_queue_size>* groove_toProcess_que{};

    // queue for receiving the ranges used to map/scale/compress velocity/offset values
    LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>*
        perVoiceSamplingThresh_fromGui_que;

    // queue for sending out the generated pattern (in HVO format) to other threads
    GeneratedDataQueue<settings::time_steps, settings::num_voices, settings::processor_io_queue_size>*
        GeneratedData_toProcessforPlayback_que;

    //---- Debugger -------------------------------------
    StringLockFreeQueue<settings::gui_io_queue_size>*
        text_toGui_que_for_debugging{};
    //----------------------------------------------------------------------


    // Model API for running the *.pt model
    MonotonicGrooveTransformerV1 modelAPI;

};

#endif //JUCECMAKEREPO_MODELTHREAD_H
