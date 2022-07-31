#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "settings.h"
#include "Representations.h"
#include "queue62.hpp"
#include <torch/torch.h>

using namespace std;

class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // void getStateInformation(juce::MemoryBlock& destData) override;
    // void setStateInformation(const void* data, int sizeInBytes) override;

    // single-producer/single-consumer buffers
    // TODO Replace spsc_queue with an AbstractFIFO implementation
    spsc_queue<juce::MidiMessage, settings::midi_queue_size> midi_message_que;
    spsc_queue<juce::AudioPlayHead::CurrentPositionInfo, settings::playhead_queue_size> playhead_que;
    spsc_queue<Note, settings::note_queue_size> note_que;
    spsc_queue<MidiMsgPlayHead, settings::midi_queue_size> midiMsgPlayhead_que;
    spsc_queue<torch::Tensor, settings::torch_tensor_queue_size> torchTensor_que;

private:

    juce::MidiBuffer tempBuffer;
    // vector<float> in_data {1, 2, 3};
    //vector<float> out_data {0, 0, 0};

};
