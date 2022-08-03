#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProcessorMethods.h"

// #include "settings.h"
// #include "Representations.h"

// #include <torch/torch.h>
#include "MonotonicGrooveTransformerV1.h"

int cnt = 0;

MidiFXProcessor::MidiFXProcessor(){
}

void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& /*buffer*/,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    if (not midiMessages.isEmpty())
    {
        // Add Midi Message to LockFreeQueue
        // place_midi_message_in_queue(midiMessages, &midi_message_que);

        // Add Playhead info to LockFreeQueue
        auto playhead = getPlayHead();
        // place_playhead_info_in_queue(playhead, &playhead_que);

        // Add note and onset to note_que using Note structure
        place_note_in_queue(midiMessages, playhead, &note_que);


        // Add a random torch tensor to queue
        // torchTensor_que.push(torch::rand({2, 3}));
    }

    if (cnt == 0)
    {

        // Following lines are for testing only!! model needs to run on a separate thread of the processor
        MonotonicGrooveTransformerV1 modelAPI(settings::default_model_path,
                                              settings::time_steps,  settings::num_voices);

        modelAPI.forward_pass(torch::rand({settings::time_steps, settings::num_voices * 3}));

        auto hits_probabilities = modelAPI.get_hits_probabilities();

        auto [hits, velocities, offsets] = modelAPI.sample("Threshold");


        /*DBG(tensor2string(velocities));
        DBG(tensor2string(offsets));*/


        auto txt = string("hits_probabilities");
        text_message_queue.WriteTo(&txt, 1);
        txt = string(tensor2string(hits_probabilities));
        text_message_queue.WriteTo(&txt,1);
        /*txt = string("clear");
        text_message_queue.WriteTo(&txt,1);
        txt = string("!!!!!");
        MidiFXProcessorPointer.text_message_queue.WriteTo(&txt,1);
        */

        /*
    showMessageinEditor(MidiFXProcessorPointer.text_message_queue,
                        tensor2string(hits_probabilities), "hits_probabilities", true);*/
        cnt +=1;
        DBG(juce::String(cnt));
    }

    midiMessages.swapWith(tempBuffer);
}

juce::AudioProcessorEditor* MidiFXProcessor::createEditor()
{
    return new MidiFXProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiFXProcessor();
}
