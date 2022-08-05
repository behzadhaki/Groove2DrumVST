#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "settings.h"
#include "Includes/Representations.h"
#include "Includes/LockFreeQueueTemplate.h"
#include <torch/torch.h>
#include "Model/ModelAPI.h"
#include "ProcessingThreads/GrooveThread.h"

using namespace std;

class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    ~MidiFXProcessor() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // single-producer/single-consumer queues
    // for inter-Thread Communication
    LockFreeQueue<Note, settings::note_queue_size> incoming_note_que;  // used to communicate with GrooveThread
    LockFreeQueue<Note, settings::note_queue_size> note_que;        // used to communicate with note logger
    LockFreeQueue<string, settings::text_message_queue_size> text_message_queue;


    // THreads
    GrooveThread groove_thread;
    bool groove_thread_ready;
private:

    juce::MidiBuffer tempBuffer;
    MonotonicGrooveTransformerV1 modelAPI;

};
