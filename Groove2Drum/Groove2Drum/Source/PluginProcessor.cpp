#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"


MidiFXProcessor::MidiFXProcessor(){
    modelAPI = MonotonicGrooveTransformerV1 (settings::default_model_path, settings::time_steps,  settings::num_voices);
}

void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    if (not midiMessages.isEmpty())
    {
        // STEP 1
        // get Playhead info and Add note and onset to note_que using Note structure
        auto playhead = getPlayHead();

        // send notes to the GrooveThread and also gui logger for notes
        place_note_in_queue(midiMessages, playhead, &incoming_note_que);
        place_note_in_queue(midiMessages, playhead, &note_que);

        /*modelAPI.forward_pass(torch::rand({settings::time_steps, settings::num_voices * 3}));
        auto hits_probabilities = modelAPI.get_hits_probabilities();
        auto [hits, velocities, offsets] = modelAPI.sample("Threshold");
        showMessageinEditor(&text_message_queue,
                            string(tensor2string(hits_probabilities)),
                            "hits_probabilities",
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
