#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "settings.h"
#include "Representations.h"
#include "queue62.hpp"
#include "AbstractFifos.h"
#include <torch/torch.h>

using namespace std;

class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // single-producer/single-consumer buffers
    // A lockFree AbstractFifo queue holding
    NoteQueue note_que;

    spsc_queue<torch::Tensor, settings::torch_tensor_queue_size> torchTensor_que;

private:

    juce::MidiBuffer tempBuffer;
    // vector<float> in_data {1, 2, 3};
    //vector<float> out_data {0, 0, 0};

};
