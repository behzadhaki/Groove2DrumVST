//
// Created by Behzad Haki on 2022-08-13.
//

#ifndef JUCECMAKEREPO_MODELTHREAD_H
#define JUCECMAKEREPO_MODELTHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructs.h"
#include "../InterThreadFifos.h"
#include "../settings.h"
#include "../Model/ModelAPI.h"

class ModelThread: public juce::Thread/*, public juce::ChangeBroadcaster*/
{
public:

    // constructor
    ModelThread();

    // destructor
    ~ModelThread() override;

    // run this in destructor destructing object
    void prepareToStop();

    // local array to keep track of !!NEW!! sampling thresholds
    // although empty here, remember model is initialized using
    // default_sampling_thresholds in ../settings.h
    vector<float> perVoiceSamplingThresholds {nine_voice_kit_default_sampling_thresholds};
    vector<float> perVoiceMaxNumVoicesAllowed {nine_voice_kit_default_max_voices_allowed};

    void startThreadUsingProvidedResources(
        IntraProcessorFifos::GrooveThreadToModelThreadQues* GrooveThreadToModelThreadQuesPntr,
        IntraProcessorFifos::ModelThreadToProcessBlockQues* ModelThreadToProcessBlockQuesPntr,
        GuiIOFifos::ModelThreadToDrumPianoRollWidgetQues* ModelThreadToDrumPianoRollWidgetQuesPntr,
        GuiIOFifos::DrumPianoRollWidgetToModelThreadQues* DrumPianoRollWidgetToModelThreadQuesPntr);



    // don't call this from the parent thread
    // instead, use startThread from which
    // run() is implicitely called
    void run() override;

    // Used to check if thread is ready to be stopped
    // used to check if a parent thread has externally
    // requested the thread to stop
    bool readyToStop;

    // generated_hvo to be sent to next thread
    HVO<HVO_params::time_steps, HVO_params::num_voices > generated_hvo;

    // placeholder for reading the latest groove received in queue
    MonotonicGroove<HVO_params::time_steps> scaled_groove;

private:

    // Intra Processor Queues
    IntraProcessorFifos::GrooveThreadToModelThreadQues* GrooveThreadToModelThreadQues;
    IntraProcessorFifos::ModelThreadToProcessBlockQues* ModelThreadToProcessBlockQues;

    // Inter GUI Processor Queues
    GuiIOFifos::ModelThreadToDrumPianoRollWidgetQues* ModelThreadToDrumPianoRollWidgetQues;
    GuiIOFifos::DrumPianoRollWidgetToModelThreadQues* DrumPianoRollWidgetToModelThreadQues;

    // Model API for running the *.pt model
    MonotonicGrooveTransformerV1 modelAPI;

};

#endif //JUCECMAKEREPO_MODELTHREAD_H
