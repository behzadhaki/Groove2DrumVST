#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"
#include "MonotonicGrooveTransformerV1.h"


#include <iostream>

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{
    // Sample rate text
    addAndMakeVisible(SampleRateLabel);
    SampleRateLabel.setText("MAKE SURE SAMPLE RATE IS "+juce::String(settings::sample_rate), juce::dontSendNotification);
    SampleRateLabel.setBounds (0, 0, 300, 30);

    // Create TextEditor for Note Struct
    addAndMakeVisible (NoteStructLoggerTextEditor);
    NoteStructLoggerTextEditor.setMultiLine (true);
    NoteStructLoggerTextEditor.setBounds (100, 40, 500, 100);
    NoteStructLoggerTextEditor.start_Thread(MidiFXProcessorPointer.note_que);

    // Create TextEditor for Text Messages
    addAndMakeVisible (TextMessageLoggerTextEditor);
    TextMessageLoggerTextEditor.setMultiLine (true);
    TextMessageLoggerTextEditor.setBounds (100, 200, 500, 100);
    TextMessageLoggerTextEditor.start_Thread(MidiFXProcessorPointer.text_message_queue);


    // Set window size
    setSize (620, 500);

    // Following lines are for testing only!! model needs to run on a separate thread of the processor
    MonotonicGrooveTransformerV1 modelAPI(settings::default_model_path,
                                          settings::time_steps,  settings::num_voices);

    modelAPI.forward_pass(torch::rand({settings::time_steps, settings::num_voices * 3}));

    auto hits_probabilities = modelAPI.get_hits_probabilities();

    auto [hits, velocities, offsets] = modelAPI.sample("Threshold");

    /*
     auto hits_probabilities = modelAPI.get_hits_probabilities();
     auto velocities = modelAPI.get_velocities();
    auto offsets = modelAPI.get_offsets();DBG(tensor2string(hits));
    DBG(tensor2string(velocities));
    DBG(tensor2string(offsets));
    */

    /*auto txt = string("hits_probabilities");
    MidiFXProcessorPointer.text_message_queue.WriteTo(&txt, 1);
    txt = string(tensor2string(hits_probabilities));
    MidiFXProcessorPointer.text_message_queue.WriteTo(&txt,1);
    txt = string("clear");
    MidiFXProcessorPointer.text_message_queue.WriteTo(&txt,1);
    txt = string("!!!!!");
    MidiFXProcessorPointer.text_message_queue.WriteTo(&txt,1);
    */

    /*
    showMessageinEditor(MidiFXProcessorPointer.text_message_queue,
                        tensor2string(hits_probabilities), "hits_probabilities", true);*/
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


