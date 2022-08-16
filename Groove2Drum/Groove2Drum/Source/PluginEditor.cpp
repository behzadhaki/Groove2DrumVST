#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

MidiFXProcessorEditor::MidiFXProcessorEditor(MidiFXProcessor& MidiFXProcessorPointer)
    : AudioProcessorEditor(&MidiFXProcessorPointer)
{
    // Sample rate text
    SampleRateLabel.setText("MAKE SURE SAMPLE RATE IS "+juce::String(settings::sample_rate)+
                            " & BUFFER SIZE IS AT MOST "+ juce::String(settings::largest_buffer_size)
                                , juce::dontSendNotification);
    SampleRateLabel.setColour(SampleRateLabel.backgroundColourId, juce::Colour(220, 100, 60));
    SampleRateLabel.setBounds (0, 0, 400, 30);
    addAndMakeVisible(SampleRateLabel);

    // Create TextEditor for BasicNote Struct
    addAndMakeVisible (BasicNoteStructLoggerTextEditor);
    BasicNoteStructLoggerTextEditor.setMultiLine (true);
    BasicNoteStructLoggerTextEditor.setBounds (100, 40, 500, 100);
    BasicNoteStructLoggerTextEditor.startThreadUsingProvidedResources(MidiFXProcessorPointer.note_toGui_que.get());

    // Create TextEditor for Text Messages
    addAndMakeVisible (TextMessageLoggerTextEditor);
    TextMessageLoggerTextEditor.setMultiLine (true);
    TextMessageLoggerTextEditor.setBounds (100, 200, 500, 100);
    TextMessageLoggerTextEditor.startThreadUsingProvidedResources(MidiFXProcessorPointer.text_toGui_que.get());


    // Set window size
    setSize (620, 500);
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


