#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProcessorMethods.h"

// #include "settings.h"
// #include "Representations.h"

// #include <torch/torch.h>

MidiFXProcessor::MidiFXProcessor(){}

void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& /*buffer*/,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    if (not midiMessages.isEmpty())
    {
        // Add Midi Message to Queue
        place_midi_message_in_queue(midiMessages, &midi_message_que);

        // Add Playhead info to Queue
        auto playhead = getPlayHead();
        place_playhead_info_in_queue(playhead, &playhead_que);

        // Add note and onset to note_que using Note structure
        place_note_in_queue(midiMessages, playhead, &note_que);

        // Add midi message and playhead to midiMsgPlayhead_que
        place_midiMessagePlayhead_in_queue(midiMessages, playhead, &midiMsgPlayhead_que);

        // Add a random torch tensor to queue
        // torchTensor_que.push(torch::rand({2, 3}));

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
