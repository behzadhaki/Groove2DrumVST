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
    basicNoteStructLoggerTextEditor = MidiFXProcessorPointer.basicNoteStructLoggerTextEditor.get();
    addAndMakeVisible (basicNoteStructLoggerTextEditor);

    // Create TextEditor for Text Messages
    textMessageLoggerTextEditor = MidiFXProcessorPointer.textMessageLoggerTextEditor.get();
    addAndMakeVisible (MidiFXProcessorPointer.textMessageLoggerTextEditor.get());

    // Create TextEditor for Text Messages
    textMessageLoggerTextEditor_mainprocessBlockOnly = MidiFXProcessorPointer.textMessageLoggerTextEditor_mainprocessBlockOnly.get();
    addAndMakeVisible (MidiFXProcessorPointer.textMessageLoggerTextEditor_mainprocessBlockOnly.get());

    // Set window size
    setSize (620, 600);
}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


