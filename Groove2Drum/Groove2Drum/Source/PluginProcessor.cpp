#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "ProcessingThreads/GrooveThread.h"
#include <cstdlib> // for sleep()

using namespace std;


MidiFXProcessor::MidiFXProcessor(){

    //groove_thread_ready = false;

    //editor queues
    note_toGui_que = make_unique<LockFreeQueue<BasicNote, settings::gui_io_queue_size>>();
    text_toGui_que = make_unique<StringLockFreeQueue<settings::gui_io_queue_size>>();

    // control paramer queues
    veloff_fromGui_que = make_unique<LockFreeQueue<array<float, 4>, gui_io_queue_size>>();
    perVoiceSamplingThresh_fromGui_que =
        make_unique<LockFreeQueue<std::array<float, settings::num_voices>,
            settings::gui_io_queue_size>>();


    //groove thread params
    note_toProcess_que =
        make_unique<LockFreeQueue<BasicNote, settings::processor_io_queue_size>>();
    groove_toProcess_que =
        make_unique<MonotonicGrooveQueue<settings::time_steps,processor_io_queue_size>>();

    HVO_toProcessforPlayback_que =
        make_unique<HVOQueue<settings::time_steps, settings::num_voices,
                             settings::processor_io_queue_size>>();

    // queue for displaying the monotonicgroove in editor
    groove_toGui_que = make_unique<MonotonicGrooveQueue<settings::time_steps,
                                                       gui_io_queue_size>>();


    // give access to resources and run threads
    grooveThread.startThreadUsingProvidedResources(
        note_toProcess_que.get(),
        groove_toProcess_que.get(),
        veloff_fromGui_que.get(),
        groove_toGui_que.get(),
        text_toGui_que.get());

    modelThread.startThreadUsingProvidedResources(
        groove_toProcess_que.get(),
        perVoiceSamplingThresh_fromGui_que.get(),
        HVO_toProcessforPlayback_que.get(),
        text_toGui_que.get());
}

// auto test_tensor = torch::randn({32, 9});

MidiFXProcessor::~MidiFXProcessor(){
    /*if (!grooveThread.readyToStop)
    {
        grooveThread.prepareToStop();
    }
    if (!modelThread.readyToStop)
    {
        modelThread.prepareToStop();
    }*/
}

void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    if (not midiMessages.isEmpty() /*and groove_thread_ready*/)
    {
        // STEP 1
        // get Playhead info and Add note and onset to note_toGui_que using BasicNote structure
        auto playhead = getPlayHead();

        // send BasicNotes to the GrooveThread and also gui logger for notes
        place_BasicNote_in_queue<settings::gui_io_queue_size>(midiMessages, playhead, note_toGui_que.get());
        place_BasicNote_in_queue<settings::processor_io_queue_size>(midiMessages, playhead, note_toProcess_que.get());
    }

    midiMessages.swapWith(tempBuffer);


    buffer.clear(); //

}

juce::AudioProcessorEditor* MidiFXProcessor::createEditor()
{
    return new MidiFXProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiFXProcessor();
}
