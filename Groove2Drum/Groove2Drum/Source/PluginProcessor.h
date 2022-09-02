#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "InterThreadFifos.h"
#include <torch/torch.h>

#include "ProcessorThreads/GrooveThread.h"
#include "ProcessorThreads/ModelThread.h"

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
    unique_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>> GrooveThread2GGroovePianoRollWidgetQue;
    unique_ptr<GuiIOFifos::GroovePianoRollWidget2GrooveThreadQues> GroovePianoRollWidget2GrooveThreadQues;

    // modelThread i.o queues w/ DrumPianoRoll Widget
    unique_ptr<HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>> ModelThreadToDrumPianoRollWidgetQue;
    unique_ptr<GuiIOFifos::DrumPianoRollWidgetToModelThreadQues> DrumPianoRollWidgetToModelThreadQues;

    // GUI THREADS HOSTED IN PROCESSOR

    // Threads used for generating patterns in the background
    GrooveThread grooveThread;
    ModelThread modelThread;

    // getters
    float get_playhead_pos();

    // APVTS
    juce::AudioProcessorValueTreeState apvts;
private:
    // =========  Queues for communicating Between the main threads in processor  =============================================
    unique_ptr<LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>> ProcessBlockToGrooveThreadQue;
    unique_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>> GrooveThreadToModelThreadQue;
    unique_ptr<GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>> ModelThreadToProcessBlockQue;


    // holds the latest generations to loop over
    GeneratedData<HVO_params::time_steps, HVO_params::num_voices> latestGeneratedData;

    // holds the playhead position for displaying on GUI
    float playhead_pos;

    // holds the previous start ppq to check restartedFlag status
    double startPpq {0};

    //  midiBuffer to fill up with generated data
    juce::MidiBuffer tempBuffer;

    // Parameter Layout for apvts
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();


};
