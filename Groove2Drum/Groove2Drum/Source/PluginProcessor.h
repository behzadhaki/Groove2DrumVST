#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "InterThreadFifos.h"
#include <torch/torch.h>

#include "ProcessingThreads/GrooveThread.h"
#include "ProcessingThreads/ModelThread.h"

// #include "gui/CustomGuiTextEditors.h"

using namespace std;


class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    ~MidiFXProcessor() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // FIFOs for communicating between GUI and processor
    unique_ptr<GuiIOFifos::ProcessorToTextEditorQues> ProcessorToTextEditorQues;

    // grooveThread i.o queues w/ GroovePianoRollWidget
    unique_ptr<GuiIOFifos::GrooveThread2GGroovePianoRollWidgetQues> GrooveThread2GGroovePianoRollWidgetQues;
    unique_ptr<GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues> GroovePianoRollWidget2GrooveThreadQues;

    // modelThread i.o queues w/ DrumPianoRoll Widget
    unique_ptr<GuiIOFifos::ModelThreadToDrumPianoRollWidgetQues> ModelThreadToDrumPianoRollWidgetQues;
    unique_ptr<GuiIOFifos::DrumPianoRollWidgetToModelThreadQues> DrumPianoRollWidgetToModelThreadQues;

    // GUI THREADS HOSTED IN PROCESSOR

    // Threads used for generating patterns in the background
    GrooveThread grooveThread;
    ModelThread modelThread;

    // getters
    float get_playhead_pos();


private:

    // Queues for communicating Between the main threads in processor
    unique_ptr<IntraProcessorFifos::ProcessBlockToGrooveThreadQues> ProcessBlockToGrooveThreadQues;
    unique_ptr<IntraProcessorFifos::GrooveThreadToModelThreadQues> GrooveThreadToModelThreadQues;
    unique_ptr<IntraProcessorFifos::ModelThreadToProcessBlockQues> ModelThreadToProcessBlockQues;

    // holds the latest generations to loop over
    GeneratedData<HVO_params::time_steps, HVO_params::num_voices> latestGeneratedData;

    // holds the playhead position for displaying on GUI
    float playhead_pos;

    //  midiBuffer to fill up with generated data
    juce::MidiBuffer tempBuffer;
};
