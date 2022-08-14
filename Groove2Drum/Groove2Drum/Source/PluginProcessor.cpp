#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "ProcessingThreads/GrooveThread.h"
#include <cstdlib> // for sleep()

using namespace std;


MidiFXProcessor::MidiFXProcessor(){

    //groove_thread_ready = false;

    //editor queues
    note_toGui_que = make_unique<LockFreeQueue<Note, settings::gui_io_queue_size>>();
    text_toGui_que = make_unique<StringLockFreeQueue<settings::gui_io_queue_size>>();

    // control paramer queues
    veloff_fromGui_que = make_unique<LockFreeQueue<array<float, 4>, gui_io_queue_size>>();
    thresholds_fromGui_que = make_unique<LockFreeQueue<std::array<float, settings::num_voices>, settings::gui_io_queue_size>>();


    //groove thread params
    note_toProcess_que = make_unique<LockFreeQueue<Note, settings::processor_io_queue_size>>();
    groove_toProcess_que = make_unique<MonotonicGrooveQueue<settings::time_steps,
                                                            processor_io_queue_size>>();

    HVO_toProcessforPlayback_que = make_unique<HVOQueue<settings::time_steps, settings::num_voices,
                                                        settings::processor_io_queue_size>>();

    // queue for displaying the monotonicgroove in editor
    groove_toGui_que = make_unique<MonotonicGrooveQueue<settings::time_steps,
                                                       gui_io_queue_size>>();


    grooveThread.giveAccesstoResources(note_toProcess_que.get(), groove_toProcess_que.get(),
                              veloff_fromGui_que.get(), groove_toGui_que.get(),
                              text_toGui_que.get());

    modelThread.giveAccesstoResources(groove_toProcess_que.get(), thresholds_fromGui_que.get(),
                                      HVO_toProcessforPlayback_que.get(), text_toGui_que.get());


}

// auto test_tensor = torch::randn({32, 9});

MidiFXProcessor::~MidiFXProcessor(){
    //groove_thread.prepareToStop();
}


void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    if (not midiMessages.isEmpty() /*and groove_thread_ready*/)
    {
        // STEP 1
        // get Playhead info and Add note and onset to note_toGui_que using Note structure
        auto playhead = getPlayHead();

        // send notes to the GrooveThread and also gui logger for notes
        place_note_in_queue<settings::gui_io_queue_size>(midiMessages, playhead, note_toGui_que.get());
        place_note_in_queue<settings::processor_io_queue_size>(midiMessages, playhead, note_toProcess_que.get());



        /*modelAPI.forward_pass(torch::rand({settings::time_steps, settings::num_voices * 3}));
        auto hits_probabilities = modelAPI.get_hits_probabilities();
        auto [hits, velocities, offsets] = modelAPI.sample("Threshold");


         showMessageinEditor(&text_toGui_que,
                            string(tensor2string(hits_probabilities)),
                            "hits_probabilities",
                            false);*/

        /*showMessageinEditor(&text_toGui_que,
                            string("NoteReceived \t"),
                            "MESSAGE: ",
                            false);*/

        //text_toGui_que->addText("TEST");
        /*showMessageinEditor(text_toGui_que.get(),
                            tensor2string(test_tensor),
                            "hits_probabilities",
                            true);

        showMessageinEditor(text_toGui_que.get(),
                            "xsdgsd",
                            "test",
                            false);*/


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
