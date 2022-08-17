//
// Created by Behzad Haki on 2022-08-13.
//

#include "ModelThread.h"
#include "../Includes/UtilityMethods.h"


ModelThread::ModelThread(): juce::Thread("Model_Thread")
{
    groove_toProcess_que = nullptr;
    perVoiceSamplingThresh_fromGui_que = nullptr;
    GeneratedData_toProcessforPlayback_que = nullptr;

    readyToStop = false;
}



ModelThread::~ModelThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }
}


void ModelThread::startThreadUsingProvidedResources(
    MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>*
        groove_toProcess_quePntr,
    LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>*
        perVoiceSamplingThresh_fromGui_quePntr,
    GeneratedDataQueue<settings::time_steps, settings::num_voices, settings::processor_io_queue_size>*
        GeneratedData_toProcessforPlayback_quePntr,
    StringLockFreeQueue<settings::gui_io_queue_size>*
        text_toGui_que_for_debuggingPntr
    )
{
    // get access to resources
    groove_toProcess_que = groove_toProcess_quePntr;
    perVoiceSamplingThresh_fromGui_que = perVoiceSamplingThresh_fromGui_quePntr;
    GeneratedData_toProcessforPlayback_que = GeneratedData_toProcessforPlayback_quePntr;
    text_toGui_que_for_debugging = text_toGui_que_for_debuggingPntr;

    // load model
    modelAPI = MonotonicGrooveTransformerV1();
    bool isLoaded = modelAPI.loadModel(
        settings::default_model_path, settings::time_steps, settings::num_voices);

    // check if model loaded successfully
    if (isLoaded)
    {
        showMessageinEditor(text_toGui_que_for_debugging,
                            modelAPI.model_path, "Model Loaded From", true);
    }
    else
    {
        showMessageinEditor(text_toGui_que_for_debugging,
                            modelAPI.model_path, "Failed to Load Model From", true);
    }

    // start thread
    startThread();
}


void ModelThread::run()
{
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    // flag to check if sampling thresholds are changed or new groove is received
    bool shouldResample;
    bool newGrooveAvailable;

    // generated_hvo to be sent to next thread
    HVO<settings::time_steps, settings::num_voices > generated_hvo;

    // placeholder for reading the latest groove received in queue
    MonotonicGroove<settings::time_steps> scaled_groove;

    // local array to keep track of !!NEW!! sampling thresholds
    // although empty here, remember model is initialized using
    // default_sampling_thresholds in ../settings.h
    array<float, settings::num_voices> perVoiceSamplingThresholds = {};

    while (!bExit)
    {
        // reset flags
        shouldResample = false;
        newGrooveAvailable = false;

        bExit = threadShouldExit();


        if (perVoiceSamplingThresh_fromGui_que != nullptr)
        {
            if (perVoiceSamplingThresh_fromGui_que->getNumReady() > 0
                   and not this->threadShouldExit()){

                perVoiceSamplingThresholds = perVoiceSamplingThresh_fromGui_que->getLatestOnly();


                std::vector<float> thresh_vec(std::begin(perVoiceSamplingThresholds),
                                             std::end(perVoiceSamplingThresholds));
                modelAPI.set_sampling_thresholds(thresh_vec);
                shouldResample = true;
            }
        }

        if (groove_toProcess_que != nullptr)
        {
            if (groove_toProcess_que->getNumReady() > 0
                   and not this->threadShouldExit())
            {
                // read latest groove
                scaled_groove = groove_toProcess_que->getLatestOnly();

                DBG("HERE NOW");

                // set flag to re-run model
                newGrooveAvailable = true;

                // TODO can comment block --- for debugging only
                /*{
                    bool showHits = false;
                    bool showVels = false;
                    bool showOffs = true;
                    bool needScaled = true;
                    showMessageinEditor(text_toGui_que_for_debugging,
                                        scaled_groove.getStringDescription(
                                            showHits, showVels, showOffs, needScaled),
                                        "Groove Offsets",
                                        true);
                }*/
            }

           if (newGrooveAvailable)
            {
                DBG("HERE NOW 2");
                // pass scaled version mapped to closed hats to input
                // !!!! dont't forget to use the scaled tensor (with modified vel/offsets)
                bool useGrooveWithModifiedVelOffset = true;
                int mapGrooveToVoiceNumber = 2;     // closed hihat
                modelAPI.forward_pass(scaled_groove.getFullVersionTensor(
                    useGrooveWithModifiedVelOffset,
                    mapGrooveToVoiceNumber));
                DBG("HERE NOW 3");
                shouldResample = true;
            }

        }

        // should resample output if, input new groove received
        if (shouldResample)
        {
            DBG("HERE NOW 4");

            auto [hits, velocities, offsets] = modelAPI.sample("Threshold");
            generated_hvo = HVO<settings::time_steps, settings::num_voices>(
                hits, velocities, offsets);

            DBG("HERE NOW 5");

            // TODO can comment block --- for debugging only
            {
                bool showHits = true;
                bool showVels = false;
                bool showOffs = false;
                bool needScaled = true;
                showMessageinEditor(text_toGui_que_for_debugging,
                                    generated_hvo.getStringDescription(
                                        showHits, showVels, showOffs, needScaled),
                                    "Generated HVO",
                                    true);
            }

            DBG("HERE NOW 6");

            // send generation to midiMessageFormatterThread
            /*for (int i = 0; i < generated_hvo.getModifiedGeneratedData().onset_ppqs.size(); i++)
                DBG("Generation " << i << " onset ppq = " << generated_hvo.getModifiedGeneratedData().onset_ppqs[i] << " | pitch = " << generated_hvo.getModifiedGeneratedData().onset_pitches[i]  << " | vel = " <<generated_hvo.getModifiedGeneratedData().onset_velocities[i]);
            */

            if (GeneratedData_toProcessforPlayback_que)
            {
                auto temp = generated_hvo.getModifiedGeneratedData();
                DBG("HERE NOW 8");

                GeneratedData_toProcessforPlayback_que->push(
                    generated_hvo.getModifiedGeneratedData());
            }
            else
                DBG("GeneratedData_toProcessforPlayback_que is Null!!");

            DBG("HERE NOW 7");

        }

        bExit = threadShouldExit();

        // avoid burning CPU, if reading is returning immediately
        sleep (thread_settings::GrooveThread::waitTimeBtnIters);
    }
}

void ModelThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::GrooveThread::waitTimeBtnIters*2);
    readyToStop = true;
}

