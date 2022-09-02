//
// Created by Behzad Haki on 2022-08-13.
//

#include "ModelThread.h"
#include "../Includes/UtilityMethods.h"

// ============================================================================================================
// ===          Preparing Thread for Running
// ============================================================================================================
// ------------------------------------------------------------------------------------------------------------
// ---         Step 1 . Construct
// ------------------------------------------------------------------------------------------------------------
ModelThread::ModelThread(): juce::Thread("Model_Thread")
{
    GrooveThreadToModelThreadQue = nullptr;
    ModelThreadToProcessBlockQue = nullptr;
    ModelThreadToDrumPianoRollWidgetQue = nullptr;
    APVTS2ModelThread_max_num_hits_Que = nullptr;
    APVTS2ModelThread_sampling_thresholds_Que = nullptr;
    readyToStop = false;
}

// ------------------------------------------------------------------------------------------------------------
// ---         Step 2 . give access to resources needed to communicate with other threads
// ------------------------------------------------------------------------------------------------------------
void ModelThread::startThreadUsingProvidedResources(
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>*
        GrooveThreadToModelThreadQuesPntr,
    GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>*
        ModelThreadToProcessBlockQuesPntr,
    HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>*
        ModelThreadToDrumPianoRollWidgetQuesPntr,
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_QuePntr,
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_QuePntr)
{
    GrooveThreadToModelThreadQue = GrooveThreadToModelThreadQuesPntr;
    ModelThreadToProcessBlockQue = ModelThreadToProcessBlockQuesPntr;
    ModelThreadToDrumPianoRollWidgetQue = ModelThreadToDrumPianoRollWidgetQuesPntr;
    APVTS2ModelThread_max_num_hits_Que = APVTS2ModelThread_max_num_hits_QuePntr;
    APVTS2ModelThread_sampling_thresholds_Que = APVTS2ModelThread_sampling_thresholds_QuePntr;

    // load model
    modelAPI = MonotonicGrooveTransformerV1();
    bool isLoaded = modelAPI.loadModel(
        GeneralSettings::default_model_path, HVO_params::time_steps, HVO_params::num_voices);

    // check if model loaded successfully
    if (isLoaded)
    {
        DBG ("Model Loaded From " + modelAPI.model_path);
        /*showMessageinEditor(text_toGui_que_for_debugging,
                            modelAPI.model_path, "Model Loaded From", true);*/
    }
    else
    {
        DBG ("Failed to Load Model From " + modelAPI.model_path);
        /*showMessageinEditor(text_toGui_que_for_debugging,
                            modelAPI.model_path, "", true);*/
    }
    startThread();
}

// ------------------------------------------------------------------------------------------------------------
// ---         Step 3 . start run() thread by calling startThread().
// ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
// ---                  (Implement what the thread does inside the run() method
// ------------------------------------------------------------------------------------------------------------
void ModelThread::run()
{
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    // flag to check if sampling thresholds are changed or new groove is received
    bool shouldResample;
    bool newGrooveAvailable;



    while (!bExit)
    {
        // reset flags
        shouldResample = false;
        newGrooveAvailable = false;

        if (APVTS2ModelThread_max_num_hits_Que != nullptr)
        {
            if (APVTS2ModelThread_max_num_hits_Que->getNumReady()>0)
            {
                auto new_counts_array = APVTS2ModelThread_max_num_hits_Que->getLatestOnly();
                perVoiceMaxNumVoicesAllowed = vector<float> {begin(new_counts_array), end(new_counts_array)};
                shouldResample = true;
            }

        }

        if (APVTS2ModelThread_sampling_thresholds_Que != nullptr)
        {
            if (APVTS2ModelThread_sampling_thresholds_Que->getNumReady()>0)
            {
                auto new_thresh_array = APVTS2ModelThread_sampling_thresholds_Que->getLatestOnly();
                vector<float> new_thresh_vect(begin(new_thresh_array), end(new_thresh_array));
                modelAPI.set_sampling_thresholds(new_thresh_vect);
                shouldResample = true;
            }
        }


        if (GrooveThreadToModelThreadQue != nullptr)
        {
            if (GrooveThreadToModelThreadQue->getNumReady() > 0
                   and not this->threadShouldExit())
            {
                // read latest groove
                scaled_groove = GrooveThreadToModelThreadQue->getLatestOnly();

                // set flag to re-run model
                newGrooveAvailable = true;

            }

           if (newGrooveAvailable)
            {
                // pass scaled version mapped to closed hats to input
                // !!!! dont't forget to use the scaled tensor (with modified vel/offsets)
                bool useGrooveWithModifiedVelOffset = true;
                int mapGrooveToVoiceNumber = 2;     // put groove in the closed hihat voice

                auto groove_tensor = scaled_groove.getFullVersionTensor(useGrooveWithModifiedVelOffset,
                                                                        mapGrooveToVoiceNumber,
                                                                        HVO_params::num_voices);
                modelAPI.forward_pass(groove_tensor);
                shouldResample = true;

                /*sendChangeMessage();*/

            }

        }

        // should resample output if, input new groove received
        if (shouldResample)
        {
            auto [hits, velocities, offsets] = modelAPI.sample("Threshold", perVoiceMaxNumVoicesAllowed);
            generated_hvo = HVO<HVO_params::time_steps, HVO_params::num_voices>(
                hits, velocities, offsets);
            auto pianoRollData = HVOLight<HVO_params::time_steps, HVO_params::num_voices>(
                hits, modelAPI.get_hits_probabilities(), velocities, offsets);


            if (ModelThreadToProcessBlockQue != nullptr)
            {
                auto temp = generated_hvo.getModifiedGeneratedData();
                ModelThreadToProcessBlockQue->push(temp);
            }

            if (ModelThreadToDrumPianoRollWidgetQue != nullptr)
            {
                ModelThreadToDrumPianoRollWidgetQue->push(
                    pianoRollData);

            }
        }

        bExit = threadShouldExit();

        // avoid burning CPU, if reading is returning immediately
        // sleep (thread_settings::ModelThread::waitTimeBtnIters);
    }
}
// ============================================================================================================


// ============================================================================================================
// ===          Preparing Thread for Stopping
// ============================================================================================================
void ModelThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::ModelThread::waitTimeBtnIters*2);
    readyToStop = true;
}

ModelThread::~ModelThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }
}
// ============================================================================================================