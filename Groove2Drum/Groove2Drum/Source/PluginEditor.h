#pragma once

#include "PluginProcessor.h"
#include "CustomLoggerTextEditors.h"

using namespace std;

class MidiFXProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiFXProcessorEditor(MidiFXProcessor&) ;
    void paint(juce::Graphics&) override;
    void resized() override {}

private:
    NoteStructLoggerTextEditor NoteStructLoggerTextEditor;
    TorchTensorTextEditor TorchTensorTextEditor;

    juce::Label SampleRateLabel;
};

