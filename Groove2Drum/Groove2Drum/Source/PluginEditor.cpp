#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "settings.h"

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
    NoteStructLoggerTextEditor.start_Thread(MidiFXProcessorPointer.note_que.get());

    // Create TextEditor for Text Messages
    addAndMakeVisible (TextMessageLoggerTextEditor);
    TextMessageLoggerTextEditor.setMultiLine (true);
    TextMessageLoggerTextEditor.setBounds (100, 200, 500, 100);
    TextMessageLoggerTextEditor.start_Thread(MidiFXProcessorPointer.text_message_queue.get());


    // Set window size
    setSize (620, 500);


}

void MidiFXProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(
        juce::ResizableWindow::backgroundColourId));
}


