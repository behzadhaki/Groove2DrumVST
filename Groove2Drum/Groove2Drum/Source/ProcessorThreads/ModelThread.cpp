//
// Created by Behzad Haki on 2022-08-13.
//

#include "ModelThread.h"
#include "../Includes/UtilityMethods.h"


ModelThread::ModelThread(): juce::Thread("Model_Thread")
{
    GrooveThreadToModelThreadQue = nullptr;
    ModelThreadToProcessBlockQue = nullptr;
    ModelThreadToDrumPianoRollWidgetQue = nullptr;
    DrumPianoRollWidgetToModelThreadQues = nullptr;

    readyToStop = false;
}


ModelThread::~ModelThread()
{
    if (not readyToStop)
    {
        prepareToStop();
    }
}


void ModelThread::startThreadUsingProvidedResources(MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQuesPntr,
                                                    GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>*  ModelThreadToProcessBlockQuesPntr,
                                                    HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>* ModelThreadToDrumPianoRollWidgetQuesPntr,
                                                    GuiIOFifos::DrumPianoRollWidgetToModelThreadQues* DrumPianoRollWidgetToModelThreadQuesPntr)
{
    GrooveThreadToModelThreadQue = GrooveThreadToModelThreadQuesPntr;
    ModelThreadToProcessBlockQue = ModelThreadToProcessBlockQuesPntr;
    ModelThreadToDrumPianoRollWidgetQue = ModelThreadToDrumPianoRollWidgetQuesPntr;
    DrumPianoRollWidgetToModelThreadQues = DrumPianoRollWidgetToModelThreadQuesPntr;

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

        if (DrumPianoRollWidgetToModelThreadQues != nullptr)
        {
            auto thresh_changed_flag_ = false;
            auto max_voice_count_changed_flag_ = false;

            for (size_t i=0; i<HVO_params::num_voices; i++)
            {
                /*if (DrumPianoRollWidgetToModelThreadQues->new_sampling_thresholds[i].getNumReady() > 0)
                {
                    perVoiceSamplingThresholds[i] = DrumPianoRollWidgetToModelThreadQues->new_sampling_thresholds[i].getLatestOnly();
                    thresh_changed_flag_ = true;
                }
                if (DrumPianoRollWidgetToModelThreadQues->new_max_number_voices[i].getNumReady() > 0)
                {
                     perVoiceMaxNumVoicesAllowed[i] = DrumPianoRollWidgetToModelThreadQues->new_max_number_voices[i].getLatestOnly();
                     max_voice_count_changed_flag_ = true;
                }*/
            }
            if (thresh_changed_flag_)
            {
                /*std::vector<float> thresh_vec(std::begin(perVoiceSamplingThresholds),
                                              std::end(perVoiceSamplingThresholds));*/
                modelAPI.set_sampling_thresholds(perVoiceSamplingThresholds);
                shouldResample = true;
            }

            if (max_voice_count_changed_flag_)
            {
                modelAPI.set_max_count_per_voice_limits(perVoiceMaxNumVoicesAllowed);
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


void ModelThread::prepareToStop()
{
    //Need to wait enough so as to ensure the run() method is over before killing thread
    this->stopThread(thread_settings::ModelThread::waitTimeBtnIters*2);
    readyToStop = true;
}

