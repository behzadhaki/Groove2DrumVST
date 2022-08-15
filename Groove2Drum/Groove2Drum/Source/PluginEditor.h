#pragma once

#include "PluginProcessor.h"
#include "gui/CustomGuiTextEditors.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    BasicNoteStructLoggerTextEditor BasicNoteStructLoggerTextEditor;
    TextMessageLoggerTextEditor TextMessageLoggerTextEditor;
    juce::Label SampleRateLabel;
};

