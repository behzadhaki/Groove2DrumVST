//
// Created by Behzad Haki on 2022-08-13.
//

#include "ModelThread.h"

ModelThread::ModelThread(): juce::Thread("Model_Thread")

{
    modelAPI = MonotonicGrooveTransformerV1 ();

    bool isLoaded = modelAPI.loadModel(settings::default_model_path, settings::time_steps,  settings::num_voices);
    if (!isLoaded)
    {
        // DBG("Couldn't Load Model from");
        // DBG(settings::default_model_path);

    }
    else
    {
        // DBG("MODEL loaded successfully from ");
        // DBG(settings::default_model_path);
    }

    groove_toProcess_que = nullptr;
    thresholds_fromGui_que = nullptr;
    HVO_toProcessforPlayback_que = nullptr;

    readyToStop = false;
}



ModelThread::~ModelThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }
}

void ModelThread::giveAccesstoResources(
    MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>*
        groove_toProcess_quePntr,
    LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>*
        thresholds_fromGui_quePntr,
    HVOQueue<settings::time_steps, settings::num_voices,
             settings::processor_io_queue_size>*
        HVO_toProcessforPlayback_quePntr,
    StringLockFreeQueue<settings::gui_io_queue_size>*
        text_toGui_que_for_debuggingPntr
    )
{
    groove_toProcess_que = groove_toProcess_quePntr;
    thresholds_fromGui_que = thresholds_fromGui_quePntr;
    HVO_toProcessforPlayback_que = HVO_toProcessforPlayback_quePntr;
    text_toGui_que_for_debugging = text_toGui_que_for_debuggingPntr;

    startThread();

}

void ModelThread::run()
{
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    // flag to check if sampling thresholds are changed or new groove is received
    bool shouldResample;
    bool newGrooveAvailable;
    // generated_hvo
    HVO<settings::time_steps, settings::num_voices > generated_hvo;
    MonotonicGroove<settings::time_steps> scaled_groove;

    array<float, settings::num_voices> threshs_array = {};

    while (!bExit)
    {
        shouldResample = false;
        newGrooveAvailable = false;

        bExit = threadShouldExit();


        if (thresholds_fromGui_que != nullptr)
        {
            while (thresholds_fromGui_que->getNumReady() > 0
                   and not this->threadShouldExit()){
                // DBG("RECEIVED NEW THRESHOLD");
                threshs_array = thresholds_fromGui_que->pop();

                std::vector<float> thres_vec(std::begin(threshs_array),
                                             std::end(threshs_array));

                modelAPI.set_sampling_thresholds(thres_vec);
                shouldResample = true;
            }
        }

        if (groove_toProcess_que != nullptr)
        {
            while (groove_toProcess_que->getNumReady() > 0
                   and not this->threadShouldExit()){
                // DBG("New Groove REceived");
                scaled_groove = groove_toProcess_que->pop();
                // DBG("SCALED GROOVE");
                // DBG(scaled_groove.getStringDescription(true));
                newGrooveAvailable = true;
            }

           if (newGrooveAvailable)
            {
                // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // FIXME SOME ERROR HAPPENS HERE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // pass scaled version mapped to closed hats to input
                modelAPI.forward_pass(scaled_groove.getFullVersionTensor(false, 2)); // todo replace false with true
                shouldResample = true;
            }

        }

        if (shouldResample)
        {

            auto [hits, velocities, offsets] = modelAPI.sample("Threshold");
            generated_hvo = HVO<settings::time_steps, settings::num_voices>(
                hits, velocities, offsets);
            HVO_toProcessforPlayback_que->push(generated_hvo);

            text_toGui_que_for_debugging->addText(generated_hvo.getStringDescription(true));

        }

        bExit = threadShouldExit();

        sleep (thread_settings::GrooveThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
    }

}

void ModelThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
    readyToStop = true;
}

