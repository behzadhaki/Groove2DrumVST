#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"
#include "ProcessingThreads/GrooveThread.h"
#include <cstdlib> // for sleep()

using namespace std;


MidiFXProcessor::MidiFXProcessor(){
    modelAPI = MonotonicGrooveTransformerV1 (settings::default_model_path, settings::time_steps,  settings::num_voices);
    //groove_thread_ready = false;

    //editor queues
    note_que = make_unique<LockFreeQueue<Note, settings::note_queue_size>>();
    text_message_queue = make_unique<StringLockFreeQueue<settings::text_message_queue_size>>();

    // control paramer queues
    veloffsetScaleParamQue = make_unique<LockFreeQueue<array<float, 4>, control_params_queue_size>>();
    samplingThreshQue = make_unique<LockFreeQueue<std::vector<float>, settings::control_params_queue_size>>();


    //groove thread params
    incomingNoteQue = make_unique<LockFreeQueue<Note, settings::note_queue_size>>();
    scaledGrooveQue = make_unique<LockFreeQueue<torch::Tensor, settings::torch_tensor_queue_size>> ();


    // queue for displaying the monotonicgroove in editor
    //grooveDisplyQue = make_unique<LockFreeQueue<MonotonicGroove<time_steps>, control_params_queue_size>>();

    grooveThread.start_Thread(incomingNoteQue.get(), scaledGrooveQue.get(),
                              veloffsetScaleParamQue.get(), text_message_queue.get());


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
        // get Playhead info and Add note and onset to note_que using Note structure
        auto playhead = getPlayHead();

        // send notes to the GrooveThread and also gui logger for notes
        // place_note_in_queue(midiMessages, playhead, incoming_note_que.get());
        place_note_in_queue(midiMessages, playhead, note_que.get());
        place_note_in_queue(midiMessages, playhead, incomingNoteQue.get());



        /*modelAPI.forward_pass(torch::rand({settings::time_steps, settings::num_voices * 3}));
        auto hits_probabilities = modelAPI.get_hits_probabilities();
        auto [hits, velocities, offsets] = modelAPI.sample("Threshold");


         showMessageinEditor(&text_message_queue,
                            string(tensor2string(hits_probabilities)),
                            "hits_probabilities",
                            false);*/

        /*showMessageinEditor(&text_message_queue,
                            string("NoteReceived \t"),
                            "MESSAGE: ",
                            false);*/

        //text_message_queue->addText("TEST");
        /*showMessageinEditor(text_message_queue.get(),
                            tensor2string(test_tensor),
                            "hits_probabilities",
                            true);

        showMessageinEditor(text_message_queue.get(),
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
