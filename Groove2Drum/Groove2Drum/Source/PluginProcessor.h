#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "settings.h"
#include "Includes/CustomStructs.h"
#include "Includes/LockFreeQueueTemplate.h"
#include <torch/torch.h>

#include "ProcessingThreads/GrooveThread.h"
#include "ProcessingThreads/modelThread.h"


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
    unique_ptr<LockFreeQueue<BasicNote, settings::gui_io_queue_size>> note_toGui_que; // used to communicate with BasicNote logger
    unique_ptr<StringLockFreeQueue<settings::gui_io_queue_size>> text_toGui_que; // used for debugging only!

    // control parameter queues (shared between threads and editor)
    unique_ptr<LockFreeQueue<array<float, 4>, settings::gui_io_queue_size>> veloff_fromGui_que;
    unique_ptr<LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>>  perVoiceSamplingThresh_fromGui_que;

    // queue for displaying the monotonicgroove in editor
    unique_ptr<MonotonicGrooveQueue<settings::time_steps,
                                    settings::gui_io_queue_size>> groove_toGui_que;


private:

    juce::MidiBuffer tempBuffer;

    // THreads
    //GrooveThread groove_thread;
    unique_ptr<LockFreeQueue<BasicNote, settings::processor_io_queue_size>>
        note_toProcess_que;
    unique_ptr<MonotonicGrooveQueue<settings::time_steps,
                                    settings::processor_io_queue_size>>
        groove_toProcess_que;
    unique_ptr<HVOQueue<settings::time_steps, settings::num_voices,
                        settings::processor_io_queue_size>>
        HVO_toProcessforPlayback_que;

    // groove thread
    GrooveThread grooveThread;

    // model thread
    ModelThread modelThread;

};
