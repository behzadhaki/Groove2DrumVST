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

class ModelThread: public juce::Thread, public juce::ChangeBroadcaster
{
public:

    // constructor
    ModelThread();

    // destructor
    ~ModelThread() override;

    // run this in destructor destructing object
    void prepareToStop();

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
