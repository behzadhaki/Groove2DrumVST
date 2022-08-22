#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "InterThreadFifos.h"
#include <torch/torch.h>

#include "ProcessingThreads/GrooveThread.h"
#include "ProcessingThreads/ModelThread.h"

#include "gui/CustomGuiTextEditors.h"

using namespace std;


class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    ~MidiFXProcessor() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // QUEUES for Communicating Back and Forth with GUI
    unique_ptr<GuiIOFifos> processorGuiFIFOS;

private:

    juce::MidiBuffer tempBuffer;

    // FIFOs for communicating data between major processor threads (NOT GUI)
    unique_ptr<WithinMidiFXProcessorFifos> withinMidiFXProcessorFIFOs;

    // holds the latest generations to loop over
    GeneratedData<HVO_params::time_steps, HVO_params::num_voices> latestGeneratedData;

    // groove thread
    GrooveThread grooveThread;

    // model thread
    ModelThread modelThread;

};
