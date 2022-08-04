#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "settings.h"
#include "includes/Representations.h"
#include "includes/LockFreeQueueTemplate.h"
#include <torch/torch.h>
#include "model/ModelAPI.h"

using namespace std;

class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void placeInMBufferCPosQueue(juce::MidiBuffer mBuffer,
                                 juce::AudioPlayHead* playheadP);

    juce::AudioProcessorEditor* createEditor() override;

    // single-producer/single-consumer buffers
    // A lockFree AbstractFifo queue holding
    // LockFreeQueue<Note, settings::note_queue_size> note_que;
    LockFreeQueue<MidiBufferCurrentPos, settings::MidiBufferCurrentPos_size>
        MidiBufferCurrentPosQueue;

    LockFreeQueue<Note, settings::note_queue_size> note_que;  // MUST BE INITIALIZED IN CONSTRUCTOR!!!!!
    LockFreeQueue<string, settings::text_message_queue_size> text_message_queue;


private:

    MidiBufferCurrentPos tempMidiBufferCurrentPos;
    juce::MidiBuffer tempBuffer;
    MonotonicGrooveTransformerV1 modelAPI;
    // vector<float> in_data {1, 2, 3};
    //vector<float> out_data {0, 0, 0};

};
